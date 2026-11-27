'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-signed-brd.js <Pascal staging directory> <world_roundtrip.wasm>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const cases = [
  ['SIG0.BRD', -32768],
  ['SIG1.BRD', -1],
  ['SIG2.BRD', 0],
  ['SIG3.BRD', 1],
  ['SIG4.BRD', 32767],
];

function compareBytes(expected, actual, label) {
  if (actual.equals(expected)) return;
  let at = 0;
  while (at < Math.min(actual.length, expected.length) && actual[at] === expected[at]) at++;
  const start = Math.max(0, at - 8);
  const end = Math.min(expected.length, at + 9);
  throw new Error(`${label}: first byte mismatch at record offset ${at}; ` +
    `Pascal=${expected.subarray(start, end).toString('hex')}; ` +
    `production=${actual.subarray(start, end).toString('hex')}; ` +
    `lengths=${expected.length}/${actual.length}`);
}

function statCountInPascalBoard(record) {
  let at = 51;
  let cells = 0;
  while (cells < 1500) {
    if (at + 3 > record.length) throw new Error('Pascal signed .BRD ended inside its RLE tile section.');
    const count = record[at];
    at += 3;
    cells += count === 0 ? 1 : count;
  }
  const statCountOffset = at + 86;
  if (statCountOffset + 2 > record.length) throw new Error('Pascal signed .BRD ended before its stat count.');
  const statCount = record.readInt16LE(statCountOffset);
  if (statCount < 0 || statCount > 151) throw new Error(`Unexpected Pascal signed .BRD stat count ${statCount}.`);
  return statCount;
}

(async () => {
  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instantiated = await WebAssembly.instantiate(wasm, {});
  const e = instantiated.instance || instantiated;
  if (typeof e.exports.run_editor_brd_signed_field_roundtrip !== 'function') {
    throw new Error('Shared serializer harness lacks run_editor_brd_signed_field_roundtrip.');
  }

  for (const [filename, value] of cases) {
    const file = fs.readFileSync(path.join(stage, filename));
    if (file.length < 2 || file.readUInt16LE(0) !== file.length - 2) {
      throw new Error(`${filename}: Pascal .BRD length word does not match the record payload.`);
    }
    const record = file.subarray(2);
    const statCount = statCountInPascalBoard(record);
    new Uint8Array(e.exports.memory.buffer, e.exports.input_ptr(), file.length).set(file);
    const result = e.exports.run_editor_brd_signed_field_roundtrip(file.length, value);
    if (result !== 0) {
      throw new Error(`${filename}: production did not decode ${value} in every signed field (result ${result}).`);
    }
    const production = Buffer.from(
      new Uint8Array(e.exports.memory.buffer, e.exports.output_ptr(), e.exports.output_len()).slice(),
    );
    compareBytes(record, production, `${filename} signed-field .BRD round-trip`);
    console.log(`${filename}: Pascal ${value} decoded in time limit and six signed stat fields across ${statCount + 1} records; ${record.length} record bytes match.`);
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
