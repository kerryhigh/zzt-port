'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3] || !process.argv[4]) {
  throw new Error('Usage: compare-pascal-signed-world.js <Pascal staging directory> <world_roundtrip.wasm> <DEMO.ZZT>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const sourceWorldPath = path.resolve(process.argv[4]);
const cases = [
  ['WRS0.SAV', -32768],
  ['WRS1.SAV', -1],
  ['WRS2.SAV', 0],
  ['WRS3.SAV', 1],
  ['WRS4.SAV', 32767],
];

function compareBytes(expected, actual, label) {
  if (actual.equals(expected)) return;
  let at = 0;
  while (at < Math.min(actual.length, expected.length) && actual[at] === expected[at]) at++;
  const start = Math.max(0, at - 8);
  const end = Math.min(expected.length, at + 9);
  throw new Error(`${label}: first byte mismatch at file offset ${at}; ` +
    `Pascal=${expected.subarray(start, end).toString('hex')}; ` +
    `production=${actual.subarray(start, end).toString('hex')}; ` +
    `lengths=${actual.length}/${expected.length}`);
}

(async () => {
  const sourceWorld = fs.readFileSync(sourceWorldPath);
  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instantiated = await WebAssembly.instantiate(wasm, {});
  const e = instantiated.instance || instantiated;
  const exports = e.exports;
  if (typeof exports.run_world_signed_field_read !== 'function'
      || typeof exports.run_game_signed_world_save !== 'function') {
    throw new Error('Shared serializer harness lacks signed world-field read/save functions.');
  }

  for (const [filename, value] of cases) {
    const pascalSave = fs.readFileSync(path.join(stage, filename));
    new Uint8Array(exports.memory.buffer, exports.input_ptr(), pascalSave.length).set(pascalSave);
    const readResult = exports.run_world_signed_field_read(pascalSave.length, value);
    if (readResult !== 0) {
      throw new Error(`${filename}: production did not read all nine signed world fields as ${value} (result ${readResult}).`);
    }

    new Uint8Array(exports.memory.buffer, exports.input_ptr(), sourceWorld.length).set(sourceWorld);
    const saveResult = exports.run_game_signed_world_save(sourceWorld.length, value);
    if (saveResult !== 0) {
      throw new Error(`${filename}: production signed-world save returned ${saveResult} for ${value}.`);
    }
    const production = Buffer.from(
      new Uint8Array(exports.memory.buffer, exports.output_ptr(), exports.output_len()).slice(),
    );
    compareBytes(pascalSave, production, `${filename} full .SAV differential`);
    console.log(`${filename}: Pascal ${value} decoded in nine signed world fields; full ${pascalSave.length}-byte .SAV matches production.`);
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
