const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-bear-tick.js BEARPROB.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== 3) throw new Error(`Turbo Pascal Bear tick output should contain 3 cases; got ${rows.length}`);
const pascalValues = [];
for (let i = 0; i < rows.length; i++) {
  const values = rows[i].trim().split(/\s+/).map(Number);
  if (values.length !== 4 || values.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Turbo Pascal Bear tick case ${i + 1}: ${rows[i]}`);
  }
  pascalValues.push(i + 1, ...values);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_bear_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Bear tick capture returned ${status}`);

  const pointer = instance.exports.bear_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 3 * 5);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 5);
      const field = i % 5;
      throw new Error(`Bear tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Bear tick: horizontal-first chase, aligned vertical chase, and range stop match across three cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
