const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-bomb-tick.js BOMB.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/)
  .filter((row) => row.startsWith('bomb-tick '));
const caseCount = 5;
const fieldsPerCase = 23;
if (rows.length !== caseCount) {
  throw new Error(`Turbo Pascal Bomb tick output should contain ${caseCount} cases; got ${rows.length}`);
}

const pascalValues = [];
for (let caseIndex = 0; caseIndex < caseCount; caseIndex++) {
  const [label, ...fields] = rows[caseIndex].trim().split(/\s+/);
  const values = fields.map(Number);
  if (label !== 'bomb-tick' || values.length !== fieldsPerCase
    || values.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Turbo Pascal Bomb tick case ${caseIndex + 1}: ${rows[caseIndex]}`);
  }
  if (values[0] !== caseIndex + 1) {
    throw new Error(`Turbo Pascal Bomb tick cases are out of order: ${rows[caseIndex]}`);
  }
  pascalValues.push(...values);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_bomb_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Bomb tick capture returned ${status}`);
  const pointer = instance.exports.bomb_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, caseCount * fieldsPerCase);

  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / fieldsPerCase);
      const field = i % fieldsPerCase;
      throw new Error(`Bomb tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Bomb tick: idle fuse, alternating beep, final warning/radius change, detonation/removal, and negative-fuse no-op match across five cases.');
})();
