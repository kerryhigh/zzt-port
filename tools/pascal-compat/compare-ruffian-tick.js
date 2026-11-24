const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-ruffian-tick.js RUFFIAN.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedSeeds = 'seeds 1 85 454';
if (rows[0] !== expectedSeeds) {
  throw new Error(`Unexpected Turbo Pascal Ruffian seed fixture: ${rows[0]}`);
}
const caseNames = ['wake', 'move-retain', 'move-stop'];
const expectedMaxima = [[17, 9, 2], [9, 2, 17], [9, 17, -1]];
if (rows.length !== caseNames.length + 1) {
  throw new Error(`Turbo Pascal Ruffian output should contain ${caseNames.length} cases and a seed row; got ${rows.length - 1} cases`);
}

const pascalValues = [];
for (let i = 0; i < caseNames.length; i++) {
  const row = rows[i + 1];
  const match = row.match(/^(wake|move-retain|move-stop) draws (.*?) player (-?\d+) (-?\d+) ruffian (-?\d+) (-?\d+) step (-?\d+) (-?\d+) under (-?\d+) (-?\d+) tiles (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+)$/);
  if (!match) throw new Error(`Malformed Turbo Pascal Ruffian tick case ${i + 1}: ${row}`);
  if (match[1] !== caseNames[i]) {
    throw new Error(`Unexpected Turbo Pascal Ruffian case order at ${i + 1}: ${match[1]}`);
  }

  const draws = match[2].trim().split(/\s+/).map(Number);
  const expectedDrawCount = i === 2 ? 2 : 3;
  if (draws.length !== expectedDrawCount) {
    throw new Error(`Turbo Pascal Ruffian case ${i + 1} should make ${expectedDrawCount} random draws; got ${draws.length}`);
  }
  const paddedDraws = [...draws, ...Array(3 - draws.length).fill(-1)];
  const state = match.slice(3).map(Number);
  pascalValues.push(i + 1, draws.length, ...paddedDraws, ...expectedMaxima[i], ...state);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_ruffian_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Ruffian tick capture returned ${status}`);

  const pointer = instance.exports.ruffian_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 3 * 22);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 22);
      const field = i % 22;
      throw new Error(`Ruffian tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Ruffian tick: wake, continuing movement, stop threshold, random draws, and tile/colors match across three cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
