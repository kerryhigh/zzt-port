'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-world-booleans.js <Pascal staging directory> <world_roundtrip.wasm>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const cases = [
  ['BLS0.SAV', 0],
  ['BLS1.SAV', 1],
  ['BLS2.SAV', 2],
  ['BLS3.SAV', 255],
];
const keyStart = 8;
const isSaveOffset = 264;
const reservedStart = 265;
const reservedPattern = [0, 1, 2, 255];

function compareBytes(expected, actual, label) {
  if (actual.equals(expected)) return;
  let at = 0;
  while (at < Math.min(actual.length, expected.length) && actual[at] === expected[at]) at++;
  const start = Math.max(0, at - 8);
  const end = Math.min(expected.length, at + 9);
  throw new Error(`${label}: first file-byte mismatch at ${at}; ` +
    `Pascal=${expected.subarray(start, end).toString('hex')}; ` +
    `production=${actual.subarray(start, end).toString('hex')}; ` +
    `lengths=${expected.length}/${actual.length}`);
}

(async () => {
  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instantiated = await WebAssembly.instantiate(wasm, {});
  const e = instantiated.instance || instantiated;
  const { memory, input_ptr, output_ptr, output_len, run_roundtrip } = e.exports;
  if (typeof run_roundtrip !== 'function') {
    throw new Error('Shared serializer harness lacks run_roundtrip.');
  }

  for (const [filename, rawBoolean] of cases) {
    const file = fs.readFileSync(path.join(stage, filename));
    if (file.length !== 26465) throw new Error(`${filename}: expected 26465 bytes, got ${file.length}.`);
    if (file.readUInt16LE(0) !== 0xFFFF) throw new Error(`${filename}: expected the DOS -1 world version marker.`);
    for (let index = 0; index < 7; index++) {
      if (file[keyStart + index] !== rawBoolean) {
        throw new Error(`${filename}: Pascal key Boolean byte ${index + 1} differs from raw value ${rawBoolean}.`);
      }
    }
    if (file[isSaveOffset] !== rawBoolean) {
      throw new Error(`${filename}: Pascal IsSave byte differs from raw value ${rawBoolean}.`);
    }
    for (let index = 0; index < 14; index++) {
      if (file[reservedStart + index] !== reservedPattern[index % reservedPattern.length]) {
        throw new Error(`${filename}: WorldInfo reserved byte ${index} differs from the direct Pascal setup.`);
      }
    }

    new Uint8Array(memory.buffer, input_ptr(), file.length).set(file);
    const result = run_roundtrip(file.length);
    if (result !== 0) throw new Error(`${filename}: production .SAV read/board round-trip returned ${result}.`);
    const production = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    compareBytes(file, production, `${filename} full .SAV differential`);
    console.log(`${filename}: seven key bytes, IsSave ${rawBoolean}, 14 reserved bytes, and all ${file.length} file bytes match production.`);
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
