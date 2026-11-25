'use strict';

const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-boardenc.js <BOARDENC.BRD> <world_roundtrip.wasm>');
}
const brdPath = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);

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
  if (!production.equals(sourceRecord)) {
    let at = 0;
    while (at < Math.min(production.length, sourceRecord.length) && production[at] === sourceRecord[at]) at++;
    const start = Math.max(0, at - 8);
    const end = Math.min(sourceRecord.length, at + 9);
    throw new Error(`Pascal BoardClose/shared .BRD round-trip first byte mismatch at record offset ${at}; ` +
      `Pascal=${sourceRecord.subarray(start, end).toString('hex')}; ` +
      `production=${production.subarray(start, end).toString('hex')}; ` +
      `lengths=${sourceRecord.length}/${production.length}`);
  }

  const digest = crypto.createHash('sha256').update(external).digest('hex');
  console.log(`E2-02 direct .BRD differential: compiled Pascal BoardClose record round-tripped through shared LibZoo byte-for-byte (${recordLength} record bytes; SHA-256 ${digest}).`);
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
