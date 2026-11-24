const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-lion-tick.js LION.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedSeeds = [3, 26, 0];
const expectedDrawCounts = [2, 3, 2];
const expectedMaxima = [[10, 3, -1], [10, 3, 2], [10, 2, -1]];
if (rows.length !== expectedSeeds.length) {
  throw new Error(`Turbo Pascal Lion tick output should contain 3 cases; got ${rows.length}`);
}

const pascalValues = [];
for (let i = 0; i < rows.length; i++) {
  const match = rows[i].match(/^seed (-?\d+) draws (.*?) tick (-?\d+) (-?\d+) (-?\d+) (-?\d+)$/);
  if (!match) throw new Error(`Malformed Turbo Pascal Lion tick case ${i + 1}: ${rows[i]}`);
  if (Number(match[1]) !== expectedSeeds[i]) {
    throw new Error(`Unexpected Turbo Pascal Lion seed for case ${i + 1}: ${match[1]}`);
  }
  const draws = match[2].trim().split(/\s+/).map(Number);
  if (draws.length !== expectedDrawCounts[i]) {
    throw new Error(`Turbo Pascal Lion case ${i + 1} should make ${expectedDrawCounts[i]} random draws; got ${draws.length}`);
  }
  const paddedDraws = [...draws, ...Array(3 - draws.length).fill(-1)];
  pascalValues.push(i + 1, draws.length, ...paddedDraws, ...expectedMaxima[i], ...match.slice(3).map(Number));
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_lion_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Lion tick capture returned ${status}`);

  const pointer = instance.exports.lion_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 3 * 12);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 12);
      const field = i % 12;
      throw new Error(`Lion tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Lion tick: random seeking, vertical movement, aligned seeking, and random-call order match across three cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
