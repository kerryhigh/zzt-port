const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-tiger-tick.js TIGER.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const caseNames = ['blocked-vertical', 'star-horizontal'];
if (rows.length !== caseNames.length) {
  throw new Error(`Turbo Pascal Tiger tick output should contain ${caseNames.length} cases; got ${rows.length}`);
}

const pascalValues = [];
for (let i = 0; i < rows.length; i++) {
  const match = rows[i].match(/^(blocked-vertical|star-horizontal) seed (-?\d+) draws (-?\d+) (-?\d+) (-?\d+) count (-?\d+) tiger (-?\d+) (-?\d+) tiles (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+)(?: projectile (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+))? final-seed (-?\d+)$/);
  if (!match) throw new Error(`Malformed Turbo Pascal Tiger tick case ${i + 1}: ${rows[i]}`);
  if (match[1] !== caseNames[i] || Number(match[2]) !== 330
      || Number(match[21]) !== -1860696767) {
    throw new Error(`Unexpected Turbo Pascal Tiger tick fixture state in case ${i + 1}: ${rows[i]}`);
  }

  const projectile = match[15] === undefined
    ? Array(6).fill(-1)
    : match.slice(15, 21).map(Number);
  pascalValues.push(
    i + 1,
    3,
    ...match.slice(3, 6).map(Number),
    10, 10, 2,
    ...match.slice(6, 15).map(Number),
    ...projectile,
  );
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_tiger_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Tiger tick capture returned ${status}`);

  const pointer = instance.exports.tiger_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 2 * 23);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 23);
      const field = i % 23;
      throw new Error(`Tiger tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Tiger tick: blocked-shot fallback, random draw order, tile state, and Star projectile fields match across two cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
