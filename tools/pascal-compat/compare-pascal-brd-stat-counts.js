'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-brd-stat-counts.js <Pascal staging directory> <world_roundtrip.wasm>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const cases = [
  ['STATC0.BRD', 0],
  ['STATC1.BRD', 1],
  ['STATC149.BRD', 149],
  ['STATC150.BRD', 150],
];

function compareBytes(expected, actual, label) {
  if (actual.equals(expected)) return;
  let at = 0;
  while (at < Math.min(expected.length, actual.length) && expected[at] === actual[at]) at++;
  const start = Math.max(0, at - 8);
  const end = Math.min(expected.length, at + 9);
  throw new Error(`${label}: first record-byte mismatch at ${at}; ` +
    `Pascal=${expected.subarray(start, end).toString('hex')}; ` +
    `production=${actual.subarray(start, end).toString('hex')}; ` +
    `lengths=${expected.length}/${actual.length}`);
}

function readStatCount(record) {
  let at = 51;
  let cells = 0;
  while (cells < 1500) {
    if (at + 3 > record.length) throw new Error('Pascal .BRD ended inside its RLE tile section.');
    const count = record[at];
    at += 3;
    cells += count === 0 ? 1 : count;
  }
  at += 86;
  if (at + 2 > record.length) throw new Error('Pascal .BRD ended before its stat count.');
  return {count: record.readInt16LE(at), firstStat: at + 2};
}

(async () => {
  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instantiated = await WebAssembly.instantiate(wasm, {});
  const e = instantiated.instance || instantiated;
  const { memory, input_ptr, output_ptr, output_len, run_editor_brd_record_roundtrip } = e.exports;
  if (typeof run_editor_brd_record_roundtrip !== 'function') {
    throw new Error('Shared serializer harness lacks run_editor_brd_record_roundtrip.');
  }

  for (const [filename, expectedCount] of cases) {
    const file = fs.readFileSync(path.join(stage, filename));
    if (file.length < 2 || file.readUInt16LE(0) !== file.length - 2) {
      throw new Error(`${filename}: Pascal .BRD length word does not match the complete payload.`);
    }
    const record = file.subarray(2);
    const parsed = readStatCount(record);
    if (parsed.count !== expectedCount) {
      throw new Error(`${filename}: Pascal stat count ${parsed.count}, expected ${expectedCount}.`);
    }
    const expectedRecordBytes = 190 + (expectedCount * 33);
    if (record.length !== expectedRecordBytes) {
      throw new Error(`${filename}: expected ${expectedRecordBytes} bytes for ${expectedCount + 1} zero-script stats, got ${record.length}.`);
    }
    let at = parsed.firstStat;
    for (let statId = 0; statId <= expectedCount; statId++) {
      if (at + 33 > record.length) throw new Error(`${filename}: truncated fixed record for stat ${statId}.`);
      const dataLen = record.readInt16LE(at + 23);
      if (dataLen !== 0) throw new Error(`${filename}: stat ${statId} DataLen=${dataLen}, expected controlled zero-script record.`);
      at += 33;
    }
    if (at !== record.length) throw new Error(`${filename}: unexpected trailing record bytes at ${at}/${record.length}.`);

    new Uint8Array(memory.buffer, input_ptr(), file.length).set(file);
    const result = run_editor_brd_record_roundtrip(file.length);
    if (result !== 0) throw new Error(`${filename}: production .BRD import/export returned ${result}.`);
    const production = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    compareBytes(record, production, `${filename} full .BRD stat-count differential`);
    console.log(`${filename}: Pascal stat_count ${expectedCount}, ${expectedCount + 1} fixed stat records, and all ${record.length} bytes match production.`);
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
