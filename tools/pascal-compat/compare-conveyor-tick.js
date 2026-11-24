const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-conveyor-tick.js CONVPRB.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/)
  .filter((row) => row.startsWith('conveyor-tick '));
const caseCount = 2;
const fieldsPerCase = 22;
if (rows.length !== caseCount) {
  throw new Error(`Turbo Pascal Conveyor tick output should contain ${caseCount} cases; got ${rows.length}`);
}

const pascalValues = [];
for (let caseIndex = 0; caseIndex < caseCount; caseIndex++) {
  const [label, ...fields] = rows[caseIndex].trim().split(/\s+/);
  const values = fields.map(Number);
  if (label !== 'conveyor-tick' || values.length !== fieldsPerCase
    || values.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Turbo Pascal Conveyor tick case ${caseIndex + 1}: ${rows[caseIndex]}`);
  }
  if (values[0] !== caseIndex + 1) {
    throw new Error(`Turbo Pascal Conveyor tick cases are out of order: ${rows[caseIndex]}`);
  }
  pascalValues.push(...values);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_conveyor_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Conveyor tick capture returned ${status}`);
  const pointer = instance.exports.conveyor_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, caseCount * fieldsPerCase);

  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / fieldsPerCase);
      const field = i % fieldsPerCase;
      throw new Error(`Conveyor tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Conveyor tick: clockwise and counterclockwise eight-tile rings, boulder colors, conveyor stats, and tile colors match across two cases.');
})();
