const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-shark-tick.js SHARK.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== 3 || rows[0] !== 'Seed-zero Random(10)=0') {
  throw new Error(`Unexpected Turbo Pascal Shark tick fixture: ${rows.join(' | ')}`);
}
const pascalValues = [];
for (let i = 1; i < rows.length; i++) {
  const values = rows[i].trim().split(/\s+/).map(Number);
  if (values.length !== 6 || values.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Turbo Pascal Shark tick case ${i}: ${rows[i]}`);
  }
  pascalValues.push(i, ...values);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_shark_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Shark tick capture returned ${status}`);

  const pointer = instance.exports.shark_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 2 * 7);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 7);
      const field = i % 7;
      throw new Error(`Shark tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Shark tick: movement into water and rejection of an empty tile match across two cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
