const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-object-touch.js OBJTOUCH.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedCases = 4;
const fieldsPerCase = 8;
if (rows.length !== expectedCases) {
  throw new Error(`Turbo Pascal Object touch probe should contain ${expectedCases} cases; got ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_oop_object_touch_capture_checks();
  if (status !== 0) throw new Error(`Production Object touch capture returned ${status}`);

  const pointer = instance.exports.object_touch_output_ptr();
  const view = new DataView(instance.exports.memory.buffer);
  for (let caseIndex = 0; caseIndex < expectedCases; caseIndex++) {
    const fields = rows[caseIndex].trim().split(/\s+/).map(Number);
    if (fields.length !== fieldsPerCase || fields.some((value) => !Number.isInteger(value))) {
      throw new Error(`Malformed Turbo Pascal Object touch case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    if (fields[0] !== caseIndex + 1) {
      throw new Error(`Turbo Pascal Object touch output is out of order at case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    for (let field = 0; field < fieldsPerCase; field++) {
      const actual = view.getInt16(pointer + (caseIndex * fieldsPerCase + field) * 2, true);
      if (actual !== fields[field]) {
        throw new Error(`Object touch case ${caseIndex + 1} field ${field + 1} differs: production ${actual}, Turbo Pascal ${fields[field]}`);
      }
    }
  }

  console.log('Turbo Pascal versus production Object touch: event target, missing/locked label behavior, and source-stat independence match across four cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
