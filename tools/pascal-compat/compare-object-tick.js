const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-object-tick.js OBJTIK.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedCaseCount = 4;
const fieldsPerCase = 9;
if (rows.length !== expectedCaseCount) {
  throw new Error(`Turbo Pascal Object tick probe should contain ${expectedCaseCount} cases; got ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_oop_object_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Object tick capture returned ${status}`);

  const pointer = instance.exports.object_tick_output_ptr();
  const view = new DataView(instance.exports.memory.buffer);
  for (let caseIndex = 0; caseIndex < expectedCaseCount; caseIndex++) {
    const fields = rows[caseIndex].trim().split(/\s+/).map(Number);
    if (fields.length !== fieldsPerCase || fields.some((value) => !Number.isInteger(value))) {
      throw new Error(`Malformed Turbo Pascal Object tick case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    if (fields[0] !== caseIndex + 1) {
      throw new Error(`Turbo Pascal Object tick output is out of order at case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    for (let field = 0; field < fieldsPerCase; field++) {
      const actual = view.getInt16(pointer + (caseIndex * fieldsPerCase + field) * 2, true);
      if (actual !== fields[field]) {
        throw new Error(`Object tick case ${caseIndex + 1} field ${field + 1} differs: production ${actual}, Turbo Pascal ${fields[field]}`);
      }
    }
  }

  console.log('Turbo Pascal versus production ElementObjectTick: script execution, #WALK movement, blocked THUD dispatch, and movement with DataPos < 0 match across four cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
