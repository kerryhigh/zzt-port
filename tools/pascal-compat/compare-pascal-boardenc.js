'use strict';

const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3] || !process.argv[4] || !process.argv[5]) {
  throw new Error('Usage: compare-pascal-boardenc.js <BOARDENC.BRD> <world_roundtrip.wasm> <DEMO.ZZT> <BOARDENC.ZZT>');
}
const brdPath = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const worldPath = path.resolve(process.argv[4]);
const pascalWorldPath = path.resolve(process.argv[5]);

function compareBytes(expected, actual, label) {
  if (actual.equals(expected)) return;
  let at = 0;
  while (at < Math.min(actual.length, expected.length) && actual[at] === expected[at]) at++;
  const start = Math.max(0, at - 8);
  const end = Math.min(expected.length, at + 9);
  throw new Error(`${label} first byte mismatch at record offset ${at}; ` +
    `Pascal=${expected.subarray(start, end).toString('hex')}; ` +
    `production=${actual.subarray(start, end).toString('hex')}; ` +
    `lengths=${expected.length}/${actual.length}`);
}

(async () => {
  const external = fs.readFileSync(brdPath);
  if (external.length < 2) throw new Error('Pascal BOARDENC.BRD is missing its two-byte length word.');
  const recordLength = external.readUInt16LE(0);
  if (recordLength !== external.length - 2) {
    throw new Error(`Pascal .BRD length word ${recordLength} does not match ${external.length - 2} record bytes.`);
  }

  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instantiated = await WebAssembly.instantiate(wasm, {});
  const instance = instantiated.instance || instantiated;
  const e = instance.exports;
  if (typeof e.run_editor_brd_record_roundtrip !== 'function') {
    throw new Error('Shared serializer harness lacks run_editor_brd_record_roundtrip.');
  }
  new Uint8Array(e.memory.buffer, e.input_ptr(), external.length).set(external);
  const result = e.run_editor_brd_record_roundtrip(external.length);
  if (result !== 0) throw new Error(`Shared .BRD import/export returned ${result}.`);

  const production = Buffer.from(new Uint8Array(e.memory.buffer, e.output_ptr(), e.output_len()).slice());
  const sourceRecord = external.subarray(2);
  compareBytes(sourceRecord, production, 'Pascal BoardClose/shared .BRD parser-export');

  if (typeof e.run_pascal_boardclose_worldsave !== 'function') {
    throw new Error('Shared engine harness lacks run_pascal_boardclose_worldsave.');
  }
  const world = fs.readFileSync(worldPath);
  new Uint8Array(e.memory.buffer, e.input_ptr(), world.length).set(world);
  const closeResult = e.run_pascal_boardclose_worldsave(world.length);
  if (closeResult !== 0) throw new Error(`Shared BoardClose/WorldSave differential returned ${closeResult}.`);
  const productionWorld = Buffer.from(new Uint8Array(e.memory.buffer, e.output_ptr(), e.output_len()).slice());
  const pascalWorld = fs.readFileSync(pascalWorldPath);
  compareBytes(pascalWorld, productionWorld, 'Pascal BoardClose/WorldSave/shared zoo_io_world_write');

  const digest = crypto.createHash('sha256').update(external).digest('hex');
  const worldDigest = crypto.createHash('sha256').update(pascalWorld).digest('hex');
  console.log(`E2-02 direct BoardClose differential: compiled Pascal WorldSave and shared LibZoo world save match byte-for-byte (${pascalWorld.length} bytes; SHA-256 ${worldDigest}); Pascal's ${recordLength}-byte board record also round-trips through the shared .BRD parser/export (record SHA-256 ${digest}).`);
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
