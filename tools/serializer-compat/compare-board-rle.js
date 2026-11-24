const fs = require('node:fs');
const path = require('node:path');

(async () => {
  const pascalPath = path.resolve(process.argv[2] || 'build/pascal-compat-verification/BOARDRLE.BIN');
  const wasmPath = path.resolve(process.argv[3] || path.join(__dirname, 'world_roundtrip.wasm'));
  const pascalBytes = fs.readFileSync(pascalPath);
  const bytesPerBoard = 3001;
  const expectedBytes = 256 * bytesPerBoard;
  if (pascalBytes.length !== expectedBytes) {
    throw new Error(`Turbo Pascal RLE sweep size ${pascalBytes.length}; expected ${expectedBytes} bytes`);
  }

  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const { memory, output_ptr, output_len, run_board_rle_count } = instance.exports;
  if (!run_board_rle_count) throw new Error('Production RLE count helper is missing from the WASM module');

  for (let count = 0; count <= 255; count++) {
    const expected = pascalBytes.subarray(count * bytesPerBoard, (count + 1) * bytesPerBoard);
    const effectiveCount = count === 0 ? 256 : count;
    for (let cell = 0; cell < 1500; cell++) {
      const element = cell < effectiveCount ? 7 : 1;
      const color = cell < effectiveCount ? 8 : 2;
      const offset = cell * 2;
      if (expected[offset] !== element || expected[offset + 1] !== color) {
        throw new Error(`Turbo Pascal RLE count ${count} has unexpected tile at cell ${cell + 1}: ${expected[offset]},${expected[offset + 1]}`);
      }
    }
    if (expected[3000] !== 77) {
      throw new Error(`Turbo Pascal RLE count ${count} consumed board info incorrectly: max-shots=${expected[3000]}`);
    }

    const result = run_board_rle_count(count);
    if (result !== 0) throw new Error(`Production BoardOpen RLE count ${count} returned ${result}`);
    const actualLength = output_len();
    if (actualLength !== bytesPerBoard) throw new Error(`Production RLE count ${count} output ${actualLength} bytes`);
    const actual = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), actualLength).slice());
    if (!actual.equals(expected)) {
      let at = 0;
      while (at < bytesPerBoard && actual[at] === expected[at]) at++;
      throw new Error(`Turbo Pascal/production RLE count ${count} mismatch at snapshot byte ${at}: expected=${expected[at]}, actual=${actual[at]}`);
    }
  }

  console.log('BoardOpen RLE decoder: all 256 run-count bytes match Turbo Pascal tile maps and following board-info byte');
})().catch(error => { console.error(error); process.exitCode = 99; });
