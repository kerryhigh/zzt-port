const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-slime-tick.js SLIMEPRB.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const caseCount = 5;
const fieldsPerCase = 40;
if (rows.length !== caseCount) {
  throw new Error(`Turbo Pascal Slime probe should contain ${caseCount} cases; got ${rows.length}`);
}

const pascalValues = [];
for (let caseIndex = 0; caseIndex < caseCount; caseIndex++) {
  const values = rows[caseIndex].trim().split(/\s+/).map(Number);
  if (values.length !== fieldsPerCase || values.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Turbo Pascal Slime case ${caseIndex + 1}: expected ${fieldsPerCase} integers, got ${values.length}`);
  }
  if (values[0] !== caseIndex + 1) {
    throw new Error(`Turbo Pascal Slime output is out of order at case ${caseIndex + 1}: ${rows[caseIndex]}`);
  }
  pascalValues.push(...values);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_slime_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Slime tick capture returned ${status}`);
  const pointer = instance.exports.slime_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, caseCount * fieldsPerCase);

  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / fieldsPerCase);
      const field = i % fieldsPerCase;
      throw new Error(`Slime case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Slime tick: growth delay, four-way scan/spread order, single opening, trapped removal, and child P2 interval inheritance match across five cases.');
})();
