const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-change.js OOPCHG.OUT oop_label.wasm');
}

const width = 61;
const height = 26;
const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/)
  .map((line) => line.trim().split(/\s+/).map(Number));
if (rows.length !== width * height) {
  throw new Error(`Expected ${width * height} Pascal #CHANGE cells, found ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const exports = instance.exports;
  const result = exports.run_oop_change_capture_checks();
  if (result !== 0) throw new Error(`Production #CHANGE capture returned ${result}`);
  const pointer = exports.oop_change_output_ptr();
  const actual = new Uint8Array(exports.memory.buffer, pointer, width * height * 2);
  let changedCount = 0;

  for (let index = 0; index < rows.length; index++) {
    const [x, y, element, color] = rows[index];
    const expectedX = index % width;
    const expectedY = Math.floor(index / width);
    if (rows[index].length !== 4 || x !== expectedX || y !== expectedY) {
      throw new Error(`Malformed or unordered Pascal #CHANGE cell ${index}: ${rows[index].join(' ')}`);
    }
    const byteIndex = index * 2;
    if (actual[byteIndex] !== element || actual[byteIndex + 1] !== color) {
      throw new Error(`#CHANGE map differs at ${x},${y}: production ${actual[byteIndex]}/${actual[byteIndex + 1]}, Turbo Pascal ${element}/${color}`);
    }

    const originalColor = (x * 31 + y * 17) & 0xFF;
    let expectedElement = 0;
    let expectedColor = originalColor;
    if (x === 10 && y === 10) {
      expectedElement = 36;
      expectedColor = 0x1F;
    } else if (x === 11 && y === 10) {
      expectedElement = 4;
      expectedColor = 0x0F;
    } else if (x === 20 && y === 20) {
      expectedElement = 21;
      expectedColor = 0x3C;
    } else if (x > 0 && y > 0) {
      expectedElement = 21;
      expectedColor = originalColor || 0x0F;
      changedCount++;
    }
    if (element !== expectedElement || color !== expectedColor) {
      throw new Error(`Turbo Pascal #CHANGE case differs from expected scan behavior at ${x},${y}: ${element}/${color} != ${expectedElement}/${expectedColor}`);
    }
  }

  if (changedCount !== 1497) throw new Error(`Expected 1497 changed cells, got ${changedCount}`);
  console.log('Turbo Pascal versus production OOP #CHANGE: all 1497 matching cells, edge scan limits, nonmatching elements, and retained colors match across the 61-by-26 board map.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
