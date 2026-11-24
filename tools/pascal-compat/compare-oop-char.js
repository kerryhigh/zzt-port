const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-char.js OOPCHAR.OUT oop_label.wasm');
}

const inputValues = [-32768, -1, 0, 1, 127, 255, 256, 32767];
const pascalRows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/)
  .map((line) => line.trim().split(/\s+/).map(Number));
if (pascalRows.length !== inputValues.length) {
  throw new Error(`Expected ${inputValues.length} Pascal #CHAR rows, found ${pascalRows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const exports = instance.exports;
  const result = exports.run_oop_char_capture_checks();
  if (result !== 0) throw new Error(`Production #CHAR capture returned ${result}`);
  const pointer = exports.oop_char_output_ptr();
  const actualRows = Array.from(
    new Int16Array(exports.memory.buffer, pointer, inputValues.length * 4),
  );

  for (let index = 0; index < inputValues.length; index++) {
    const expected = pascalRows[index];
    const actual = actualRows.slice(index * 4, index * 4 + 4);
    if (expected.length !== 4 || expected[0] !== index + 1) {
      throw new Error(`Unexpected Pascal #CHAR row ${index + 1}: ${expected.join(' ')}`);
    }
    if (actual.some((value, column) => value !== expected[column])) {
      throw new Error(`Pascal/production #CHAR case ${index + 1} differs: ${expected.join(' ')} != ${actual.join(' ')}`);
    }

    const value = inputValues[index];
    const expectedGlyph = value >= 1 && value <= 255 ? value : 0xA5;
    if (expected[1] !== expectedGlyph) {
      throw new Error(`Turbo Pascal #CHAR ${value} left glyph ${expected[1]}, expected ${expectedGlyph}`);
    }
    if (expected[2] !== 3) {
      throw new Error(`Turbo Pascal #CHAR ${value} unexpectedly ran same-line #CYCLE: ${expected.join(' ')}`);
    }
  }

  console.log('Turbo Pascal versus production OOP #CHAR: signed extremes and the 0/1/255/256 boundaries preserve or replace the glyph identically; same-line command skip matches across eight cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
