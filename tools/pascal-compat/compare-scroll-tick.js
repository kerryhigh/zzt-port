const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-scroll-tick.js SCROLLTK.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== 256) {
  throw new Error(`Turbo Pascal Scroll tick should contain 256 colors; got ${rows.length}`);
}

const pascalValues = [];
for (let color = 0; color < 256; color++) {
  const match = rows[color].match(/^(\d+) (\d+)$/);
  if (!match) throw new Error(`Malformed Turbo Pascal Scroll tick case ${color}: ${rows[color]}`);
  const input = Number(match[1]);
  const result = Number(match[2]);
  const expected = color === 255 ? 0 : color < 15 ? color + 1 : 9;
  if (input !== color || result !== expected) {
    throw new Error(`Unexpected Pascal Scroll color transition ${input} -> ${result}; expected ${color} -> ${expected}`);
  }
  pascalValues.push(input, result);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_scroll_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Scroll tick capture returned ${status}`);

  const pointer = instance.exports.scroll_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 2 * 256);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const color = Math.floor(i / 2);
      const field = i % 2 === 0 ? 'input' : 'result';
      throw new Error(`Scroll tick color ${color} ${field} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Scroll tick: all 256 color bytes match, including 15-to-9 reset and 255-to-0 overflow.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
