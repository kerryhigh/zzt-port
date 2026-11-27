'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-brd-strings.js <Pascal staging directory> <world_roundtrip.wasm>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const cases = [
  ['STR0.BRD', 0],
  ['STR1.BRD', 1],
  ['STR2.BRD', 49],
  ['STR3.BRD', 50],
  ['STR4.BRD', 51],
  ['STR5.BRD', 127],
  ['STR6.BRD', 255],
  ['STR7.BRD', 50],
  ['STR8.BRD', 1],
];

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

  for (let index = 0; index < cases.length; index++) {
    const [filename, expectedLength] = cases[index];
    const file = fs.readFileSync(path.join(stage, filename));
    if (file.length < 2 || file.readUInt16LE(0) !== file.length - 2) {
      throw new Error(`${filename}: Pascal .BRD length word does not match the complete payload.`);
    }
    const record = file.subarray(2);
    if (record[0] !== expectedLength) {
      throw new Error(`${filename}: Pascal raw board-name length ${record[0]} does not equal expected ${expectedLength}.`);
    }
    if (index === 7 && !(record[1] === 0x41 && record[2] === 0 && record[3] === 0x80
        && record[4] === 0x82 && record[5] === 0xFF)) {
      throw new Error(`${filename}: embedded NUL/CP437 payload bytes differ from the direct Pascal setup.`);
    }
    if (index === 8) {
      const staleTail = Buffer.from('ZZT-STALE-TAIL-TEST', 'ascii').subarray(1);
      if (record[1] !== 0x58 || !record.subarray(2, 2 + staleTail.length).equals(staleTail)) {
        throw new Error(`${filename}: direct Pascal short-string assignment did not preserve the prior payload tail.`);
      }
    }

    new Uint8Array(memory.buffer, input_ptr(), file.length).set(file);
    const result = run_editor_brd_record_roundtrip(file.length);
    if (result !== 0) throw new Error(`${filename}: production .BRD import/export returned ${result}.`);
    const production = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    compareBytes(record, production, `${filename} full .BRD record differential`);
    console.log(`${filename}: Pascal short-string length ${expectedLength}; complete ${record.length}-byte payload matches production.`);
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
