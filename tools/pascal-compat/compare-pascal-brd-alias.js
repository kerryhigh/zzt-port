'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3] || !process.argv[4]) {
  throw new Error('Usage: compare-pascal-brd-alias.js <Pascal staging directory> <world_roundtrip.wasm> <expected output file>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const outputPath = path.resolve(process.argv[4]);

function compareBytes(expected, actual, label) {
  if (actual.equals(expected)) return;
  let at = 0;
  while (at < Math.min(expected.length, actual.length) && expected[at] === actual[at]) at++;
  const start = Math.max(0, at - 8);
  const end = Math.min(expected.length, at + 9);
  throw new Error(`${label}: first mismatch at record offset ${at}; ` +
    `Pascal=${expected.subarray(start, end).toString('hex')}; ` +
    `production=${actual.subarray(start, end).toString('hex')}; ` +
    `lengths=${expected.length}/${actual.length}`);
}

function readStats(record) {
  let at = 51;
  let cells = 0;
  while (cells < 1500) {
    if (at + 3 > record.length) throw new Error('Pascal alias .BRD ended inside the RLE tile section.');
    const count = record[at];
    at += 3;
    cells += count === 0 ? 1 : count;
  }
  at += 86;
  if (at + 2 > record.length) throw new Error('Pascal alias .BRD ended before its stat count.');
  const statCount = record.readInt16LE(at);
  at += 2;
  if (statCount < 1 || statCount > 151) throw new Error(`Unexpected Pascal stat count ${statCount}.`);
  const stats = [];
  for (let i = 0; i <= statCount; i++) {
    if (at + 33 > record.length) throw new Error(`Pascal alias .BRD ended inside stat ${i}.`);
    const dataPos = record.readInt16LE(at + 21);
    const dataLen = record.readInt16LE(at + 23);
    stats.push({ id: i, dataPos, dataLen });
    at += 33;
    if (dataLen > 0) at += dataLen;
    if (at > record.length) throw new Error(`Pascal alias .BRD ended inside stat ${i} script bytes.`);
  }
  return stats;
}

(async () => {
  const external = fs.readFileSync(path.join(stage, 'ALIAS.BRD'));
  if (external.length < 2 || external.readUInt16LE(0) !== external.length - 2) {
    throw new Error('Pascal ALIAS.BRD has an invalid two-byte length prefix.');
  }
  const record = external.subarray(2);
  const expectedLine = fs.readFileSync(outputPath, 'utf8').trim();
  const match = /^alias (\d+) (\d+) (\d+) (\d+) (-\d+)$/.exec(expectedLine);
  if (!match) throw new Error(`Unexpected BRDALI.OUT capture: ${JSON.stringify(expectedLine)}`);
  const [, boardId, sourceId, targetId, aliasLength, wireAlias] = match.map((value, index) => index === 0 ? value : Number(value));
  const stats = readStats(record);
  if (!stats[sourceId] || !stats[targetId] || sourceId >= targetId) {
    throw new Error(`Invalid Pascal alias stat indices source=${sourceId}, target=${targetId}.`);
  }
  if (stats[sourceId].dataLen !== aliasLength || stats[sourceId].dataLen <= 0) {
    throw new Error(`Source stat ${sourceId} DataLen=${stats[sourceId].dataLen}; expected ${aliasLength}.`);
  }
  if (stats[targetId].dataLen !== wireAlias || wireAlias >= 0) {
    throw new Error(`Target stat ${targetId} wire DataLen=${stats[targetId].dataLen}; expected negative alias ${wireAlias}.`);
  }

  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instantiated = await WebAssembly.instantiate(wasm, {});
  const e = instantiated.instance || instantiated;
  if (typeof e.exports.run_editor_brd_record_roundtrip !== 'function') {
    throw new Error('Shared serializer harness lacks run_editor_brd_record_roundtrip.');
  }
  new Uint8Array(e.exports.memory.buffer, e.exports.input_ptr(), external.length).set(external);
  const result = e.exports.run_editor_brd_record_roundtrip(external.length);
  if (result !== 0) throw new Error(`Production rejected Pascal alias board ${boardId} (result ${result}).`);
  const production = Buffer.from(new Uint8Array(e.exports.memory.buffer, e.exports.output_ptr(), e.exports.output_len()).slice());
  compareBytes(record, production, 'Pascal BoardClose/shared .BRD alias round-trip');
  const outputStats = readStats(production);
  if (outputStats[targetId].dataLen !== wireAlias) {
    throw new Error(`Production re-encoded target stat ${targetId} DataLen=${outputStats[targetId].dataLen}; expected ${wireAlias}.`);
  }
  console.log(`Pascal BoardClose on board ${boardId}: stat ${targetId} references earlier shared script data with wire DataLen ${wireAlias}; all ${record.length} record bytes matched production.`);
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
