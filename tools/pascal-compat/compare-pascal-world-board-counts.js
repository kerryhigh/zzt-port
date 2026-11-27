'use strict';

const fs = require('node:fs');
const path = require('node:path');

if (!process.argv[2] || !process.argv[3]) {
  throw new Error('Usage: compare-pascal-world-board-counts.js <Pascal staging directory> <world_roundtrip.wasm>');
}

const stage = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);
const counts = [0, 1, 99, 100];

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

  for (const count of counts) {
    for (const [extension, isSave] of [['.SAV', 1], ['.ZZT', 0]]) {
      const filename = `BCNT${count}${extension}`;
      const file = fs.readFileSync(path.join(stage, filename));
      if (file.readUInt16LE(0) !== 0xFFFF) {
        throw new Error(`${filename}: expected the DOS -1 world version marker.`);
      }
      const actualCount = file.readInt16LE(2);
      if (actualCount !== count) {
        throw new Error(`${filename}: Pascal serialized board count ${actualCount}, expected ${count}.`);
      }
      if (file[264] !== isSave) {
        throw new Error(`${filename}: IsSave byte is ${file[264]}, expected ${isSave}.`);
      }

      let at = 512;
      for (let boardId = 0; boardId <= count; boardId++) {
        if (at + 2 > file.length) throw new Error(`${filename}: missing length for board ${boardId}.`);
        const boardLength = file.readUInt16LE(at);
        at += 2;
        if (at + boardLength > file.length) {
          throw new Error(`${filename}: board ${boardId} length ${boardLength} exceeds the file.`);
        }
        at += boardLength;
      }
      if (at !== file.length) {
        throw new Error(`${filename}: ${file.length - at} unexplained trailing bytes after ${count + 1} boards.`);
      }

      new Uint8Array(memory.buffer, input_ptr(), file.length).set(file);
      const result = run_roundtrip(file.length);
      if (result !== 0) throw new Error(`${filename}: production .ZZT/.SAV round-trip returned ${result}.`);
      const production = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
      compareBytes(file, production, `${filename} full-file board-count differential`);
      console.log(`${filename}: BoardCount ${count}, ${count + 1} complete board records, IsSave ${isSave}, and all ${file.length} bytes match production.`);
    }
  }
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
