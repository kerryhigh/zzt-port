const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-put.js OOPPUT.OUT oop_label.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/)
  .map((line) => line.trim().split(/\s+/).map(Number));
const expected = [
  [1, 10, 9, 10, 9, 21, 0x2A],
  [2, 10, 0, 10, 0, 0, 0x2A],
  [3, 10, 24, 10, 24, 21, 0x2A],
  [4, 10, 25, 10, 24, 36, 0x0F],
  [5, 60, 10, 60, 10, 21, 0x2A],
  [6, 61, 10, 60, 10, 36, 0x0F],
  [7, 0, 10, 0, 10, 0, 0x2A],
  [8, 1, 10, 1, 10, 21, 0x2A]
];
if (rows.length !== expected.length) {
  throw new Error(`Expected ${expected.length} Pascal #PUT rows, found ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const exports = instance.exports;
  const result = exports.run_oop_put_capture_checks();
  if (result !== 0) throw new Error(`Production #PUT capture returned ${result}`);
  const pointer = exports.oop_put_output_ptr();
  const view = new DataView(exports.memory.buffer);

  for (let index = 0; index < expected.length; index++) {
    const pascal = rows[index];
    if (pascal.length !== 10 || pascal[0] !== index + 1) {
      throw new Error(`Malformed Turbo Pascal #PUT case ${index + 1}: ${pascal.join(' ')}`);
    }
    for (let field = 0; field < 10; field++) {
      const actual = view.getInt16(pointer + (index * 10 + field) * 2, true);
      if (actual !== pascal[field]) {
        throw new Error(`#PUT case ${index + 1}, field ${field + 1} differs: production ${actual}, Turbo Pascal ${pascal[field]}`);
      }
    }
    for (let field = 0; field < expected[index].length; field++) {
      if (pascal[field] !== expected[index][field]) {
        throw new Error(`Turbo Pascal #PUT edge case ${index + 1} differs from expected board rule: ${pascal.join(' ')}`);
      }
    }
    if (pascal[7] !== 3) {
      throw new Error(`Turbo Pascal #PUT case ${index + 1} unexpectedly changed Cycle: ${pascal.join(' ')}`);
    }
  }

  console.log('Turbo Pascal versus production OOP #PUT: top/bottom/left/right boundary behavior, solid placement color, and eight board-edge snapshots match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
