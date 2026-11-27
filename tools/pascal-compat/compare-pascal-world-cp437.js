'use strict';

const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-world-cp437.js <Pascal staging directory> <world_roundtrip.wasm>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const files = [['CP437.ZZT', 0], ['CP437.SAV', 1]];
const expectedBytes = Buffer.from([0x80, 0x82, 0xFF]);

function assertStringBytes(buffer, offset, capacity, expected, label) {
  if (buffer[offset] !== expected.length) {
    throw new Error(`${label}: Pascal short-string length is ${buffer[offset]}, expected ${expected.length}.`);
  }
  if (!buffer.subarray(offset + 1, offset + 1 + expected.length).equals(expected)) {
    throw new Error(`${label}: raw bytes are ${buffer.subarray(offset + 1, offset + 1 + expected.length).toString('hex')}, expected ${expected.toString('hex')}.`);
  }
  if (offset + 1 + capacity > buffer.length) throw new Error(`${label}: fixed short-string storage exceeds its containing record.`);
}

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
  if (typeof run_roundtrip !== 'function') throw new Error('Shared serializer harness lacks run_roundtrip.');
  for (const [filename, expectedIsSave] of files) {
    const file = fs.readFileSync(path.join(stage, filename));
    if (file.length !== 26465) throw new Error(`${filename}: expected 26465 bytes, got ${file.length}.`);
    if (file.readUInt16LE(0) !== 0xFFFF) throw new Error(`${filename}: expected the DOS -1 world version marker.`);
    if (file[264] !== expectedIsSave) throw new Error(`${filename}: IsSave is ${file[264]}, expected ${expectedIsSave}.`);
    assertStringBytes(file, 29, 20, expectedBytes, `${filename} world name`);
    assertStringBytes(file, 50, 20, expectedBytes, `${filename} first flag`);

    const boardCount = file.readInt16LE(2);
    const currentBoard = file.readUInt16LE(17);
    if (boardCount < 0 || currentBoard > boardCount) {
      throw new Error(`${filename}: invalid board count/current board ${boardCount}/${currentBoard}.`);
    }

    let fileOffset = 512;
    let currentRecord = null;
    for (let boardId = 0; boardId <= boardCount; boardId++) {
      if (fileOffset + 2 > file.length) throw new Error(`${filename}: missing length for board ${boardId}.`);
      const recordLength = file.readUInt16LE(fileOffset);
      const recordStart = fileOffset + 2;
      if (recordStart + recordLength > file.length) throw new Error(`${filename}: board ${boardId} exceeds the file.`);
      if (boardId === currentBoard) currentRecord = file.subarray(recordStart, recordStart + recordLength);
      fileOffset = recordStart + recordLength;
    }
    if (fileOffset !== file.length) throw new Error(`${filename}: ${file.length - fileOffset} unexplained trailing bytes.`);
    if (!currentRecord) throw new Error(`${filename}: current board ${currentBoard} was not serialized.`);
    assertStringBytes(currentRecord, 0, 50, expectedBytes, `${filename} current board name`);

    let boardOffset = 51;
    let cells = 0;
    while (cells < 1500) {
      if (boardOffset + 3 > currentRecord.length) throw new Error(`${filename}: ended inside board tile RLE.`);
      const run = currentRecord[boardOffset];
      boardOffset += 3;
      cells += run === 0 ? 1 : run;
    }
    if (cells !== 1500) throw new Error(`${filename}: RLE covers ${cells} cells, expected 1,500.`);
    assertStringBytes(currentRecord, boardOffset + 7, 58, expectedBytes, `${filename} current board message`);

    new Uint8Array(memory.buffer, input_ptr(), file.length).set(file);
    const result = run_roundtrip(file.length);
    if (result !== 0) throw new Error(`${filename}: production world read/board round-trip returned ${result}.`);
    const production = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    compareBytes(file, production, `${filename} full-file CP437 differential`);

    const sha256 = crypto.createHash('sha256').update(file).digest('hex');
    console.log(`${filename}: IsSave ${expectedIsSave}; world name, flag, current board name, and board message contain raw CP437 bytes ${expectedBytes.toString('hex')}; all ${file.length} file bytes match production; SHA-256 ${sha256}.`);
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
