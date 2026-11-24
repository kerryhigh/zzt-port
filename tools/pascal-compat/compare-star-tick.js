const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-star-tick.js STARPRB.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/)
  .filter((row) => /^tick[1-4] /.test(row));
const caseCount = 4;
const fieldsPerCase = 13;
if (rows.length !== caseCount) {
  throw new Error(`Turbo Pascal Star tick output should contain ${caseCount} cases; got ${rows.length}`);
}

const pascalValues = [];
for (let caseIndex = 0; caseIndex < 3; caseIndex++) {
  const match = rows[caseIndex].match(/^tick([1-3]) p2 (-?\d+) pos (-?\d+) (-?\d+) step (-?\d+) (-?\d+) under (-?\d+) (-?\d+) tiles (-?\d+) (-?\d+) (-?\d+) (-?\d+) count (-?\d+) seed -?\d+$/);
  if (!match) throw new Error(`Malformed Turbo Pascal Star tick case ${caseIndex + 1}: ${rows[caseIndex]}`);
  pascalValues.push(...match.slice(1).map(Number));
}
const removedMatch = rows[3].match(/^tick4 count (-?\d+) tiles (-?\d+) (-?\d+) (-?\d+) (-?\d+) seed -?\d+$/);
if (!removedMatch) throw new Error(`Malformed Turbo Pascal Star tick case 4: ${rows[3]}`);
pascalValues.push(4, -1, -1, -1, -1, -1, -1, -1,
  ...removedMatch.slice(2).map(Number), Number(removedMatch[1]));

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_star_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Star tick capture returned ${status}`);
  const pointer = instance.exports.star_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, caseCount * fieldsPerCase);

  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / fieldsPerCase);
      const field = i % fieldsPerCase;
      throw new Error(`Star tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Star tick: odd/even cadence, water move, tile colors, under-tile restoration, and removal match across four cases.');
})();
