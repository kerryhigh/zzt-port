const fs = require('node:fs');

const [pascalOutputPath, productionWasmPath] = process.argv.slice(2);
if (!pascalOutputPath || !productionWasmPath) {
  throw new Error('Usage: compare-game-clock.js SOUNDCLK.OUT game_clock.wasm');
}

const labels = [
  'before-minute-wrap', 'last-centisecond', 'minute-rollover',
  'below-threshold', 'at-threshold', 'zero-duration', 'backward-clock-step'
];
const rows = fs.readFileSync(pascalOutputPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== labels.length) {
  throw new Error(`Expected ${labels.length} Pascal clock rows; found ${rows.length}`);
}
const expected = rows.map((row, index) => {
  const fields = row.trim().split(/\s+/);
  if (fields[0] !== labels[index] || fields.length !== 3) {
    throw new Error(`Malformed Pascal system-clock row ${index + 1}: ${row}`);
  }
  return fields.slice(1).map(Number);
});

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(productionWasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const status = instance.exports.run_game_clock_checks();
  if (status !== 0) throw new Error(`Production clock probe returned ${status}`);
  const pointer = instance.exports.game_clock_output_ptr();
  const actual = new Int16Array(instance.exports.memory.buffer, pointer, labels.length * 2);
  for (let row = 0; row < labels.length; row++) {
    for (let field = 0; field < 2; field++) {
      if (actual[row * 2 + field] !== expected[row][field]) {
        throw new Error(`${labels[row]} field ${field + 1} differs: production=${actual[row * 2 + field]}, Pascal=${expected[row][field]}`);
      }
    }
  }
  console.log('Turbo Pascal versus production elapsed clock: threshold edges, zero duration, minute rollover, counter retention, and clock-step modulo behavior match across seven cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
