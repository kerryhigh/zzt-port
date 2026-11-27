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

  const pattern = Buffer.from(Array.from({ length: 58 }, (_, index) => String.fromCharCode(65 + (index % 26))).join(''), 'ascii');
  function assertStaleString(buffer, offset, capacity, first, label) {
    if (buffer[offset] !== 1) throw new Error(`${label}: Pascal short-string length is ${buffer[offset]}, expected 1.`);
    const expected = Buffer.concat([Buffer.from(first, 'ascii'), pattern.subarray(1, capacity)]);
    if (!buffer.subarray(offset + 1, offset + 1 + capacity).equals(expected)) {
      throw new Error(`${label}: Pascal short-string assignment did not preserve the old payload tail.`);
    }
  }
  function compareStaleTailWorld(filename, expectedIsSave) {
    const tailFile = fs.readFileSync(path.join(stage, filename));
    if (tailFile.length !== 26465) throw new Error(`${filename}: expected 26465 bytes, got ${tailFile.length}.`);
    if (tailFile.readUInt16LE(0) !== 0xFFFF) throw new Error(`${filename}: expected the DOS -1 world version marker.`);
    if (tailFile[264] !== expectedIsSave) throw new Error(`${filename}: IsSave byte is ${tailFile[264]}, expected ${expectedIsSave}.`);
    assertStaleString(tailFile, 29, 20, 'N', `${filename} world name`);
    assertStaleString(tailFile, 50, 20, 'F', `${filename} first flag`);

    const boardCount = tailFile.readUInt16LE(2);
    const currentBoard = tailFile.readUInt16LE(17);
    if (currentBoard > boardCount) throw new Error(`${filename}: current board ${currentBoard} exceeds board count ${boardCount}.`);
    let boardAtFile = 512;
    let board = null;
    for (let boardId = 0; boardId <= boardCount; boardId++) {
      if (boardAtFile + 2 > tailFile.length) throw new Error(`${filename}: missing length for board ${boardId}.`);
      const boardLength = tailFile.readUInt16LE(boardAtFile);
      const boardStart = boardAtFile + 2;
      if (boardStart + boardLength > tailFile.length) throw new Error(`${filename}: board ${boardId} exceeds the file.`);
      if (boardId === currentBoard) board = tailFile.subarray(boardStart, boardStart + boardLength);
      boardAtFile = boardStart + boardLength;
    }
    if (!board) throw new Error(`${filename}: current board ${currentBoard} was not serialized.`);
    assertStaleString(board, 0, 50, 'B', `${filename} board name`);
    let boardAt = 51;
    let cells = 0;
    while (cells < 1500) {
      if (boardAt + 3 > board.length) throw new Error(`${filename}: ended inside board tile RLE.`);
      const count = board[boardAt];
      boardAt += 3;
      cells += count === 0 ? 1 : count;
    }
    if (boardAt + 86 > board.length) throw new Error(`${filename}: ended before Board.Info.`);
    assertStaleString(board, boardAt + 7, 58, 'M', `${filename} board message`);

    new Uint8Array(memory.buffer, input_ptr(), tailFile.length).set(tailFile);
    const result = run_roundtrip(tailFile.length);
    if (result !== 0) throw new Error(`${filename}: production read/board round-trip returned ${result}.`);
    const production = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    compareBytes(tailFile, production, `${filename} stale-string-tail full-file differential`);
    console.log(`${filename}: IsSave ${expectedIsSave}, four direct Pascal stale tails, and all ${tailFile.length} file bytes match production.`);
  }

  compareStaleTailWorld('SAVTAIL.SAV', 1);
  compareStaleTailWorld('ZZTTAIL.ZZT', 0);
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
