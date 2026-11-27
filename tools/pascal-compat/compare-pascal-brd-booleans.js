'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-brd-booleans.js <Pascal staging directory> <world_roundtrip.wasm>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const cases = [
  ['BOL0.BRD', 0],
  ['BOL1.BRD', 1],
  ['BOL2.BRD', 2],
  ['BOL3.BRD', 255],
];
const boardInfoOffset = 69;
const reservedPattern = [0, 1, 2, 255];

function compareBytes(expected, actual, label) {
  if (actual.equals(expected)) return;
  let at = 0;
  while (at < Math.min(actual.length, expected.length) && actual[at] === expected[at]) at++;
  const start = Math.max(0, at - 8);
  const end = Math.min(expected.length, at + 9);
  throw new Error(`${label}: first record-byte mismatch at ${at}; ` +
    `Pascal=${expected.subarray(start, end).toString('hex')}; ` +
    `production=${actual.subarray(start, end).toString('hex')}; ` +
    `lengths=${expected.length}/${actual.length}`);
}

(async () => {
  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instantiated = await WebAssembly.instantiate(wasm, {});
  const e = instantiated.instance || instantiated;
  const { memory, input_ptr, output_ptr, output_len, run_editor_brd_record_roundtrip } = e.exports;
  if (typeof run_editor_brd_record_roundtrip !== 'function') {
    throw new Error('Shared serializer harness lacks run_editor_brd_record_roundtrip.');
  }

  for (const [filename, rawBoolean] of cases) {
    const file = fs.readFileSync(path.join(stage, filename));
    if (file.length < 2 || file.readUInt16LE(0) !== file.length - 2) {
      throw new Error(`${filename}: Pascal .BRD length word does not match the complete payload.`);
    }
    const record = file.subarray(2);
    if (record.length !== 289) throw new Error(`${filename}: expected a 289-byte board record, got ${record.length}.`);
    if (record[boardInfoOffset + 1] !== rawBoolean
        || record[boardInfoOffset + 6] !== rawBoolean) {
      throw new Error(`${filename}: Board.Info Boolean bytes differ from raw value ${rawBoolean}.`);
    }
    for (let index = 0; index < 16; index++) {
      if (record[boardInfoOffset + 70 + index] !== reservedPattern[index % reservedPattern.length]) {
        throw new Error(`${filename}: Board.Info reserved byte ${index} differs from the direct Pascal setup.`);
      }
    }

    new Uint8Array(memory.buffer, input_ptr(), file.length).set(file);
    const result = run_editor_brd_record_roundtrip(file.length);
    if (result !== 0) throw new Error(`${filename}: production .BRD import/export returned ${result}.`);
    const production = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    compareBytes(record, production, `${filename} full .BRD record differential`);
    console.log(`${filename}: raw Boolean ${rawBoolean}, 16 reserved bytes, and all ${record.length} record bytes match production.`);
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
