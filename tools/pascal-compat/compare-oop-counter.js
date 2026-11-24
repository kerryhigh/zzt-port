const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-counter.js OOPGIV.OUT oop_label.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedCaseCount = 24;
if (rows.length !== expectedCaseCount) {
  throw new Error(`Turbo Pascal #GIVE/#TAKE probe should contain ${expectedCaseCount} cases; got ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_oop_counter_overflow_checks();
  if (status !== 0) throw new Error(`Production OOP counter harness returned ${status}`);

  const pointer = instance.exports.oop_counter_output_ptr();
  const view = new DataView(instance.exports.memory.buffer);
  for (let caseIndex = 0; caseIndex < expectedCaseCount; caseIndex++) {
    const fields = rows[caseIndex].trim().split(/\s+/).map(Number);
    if (fields.length !== 3 || fields.some((value) => !Number.isInteger(value))) {
      throw new Error(`Malformed Turbo Pascal counter case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    if (fields[0] !== caseIndex + 1) {
      throw new Error(`Turbo Pascal counter output is out of order at case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    for (let field = 0; field < 3; field++) {
      const actual = view.getInt16(pointer + (caseIndex * 3 + field) * 2, true);
      if (actual !== fields[field]) {
        throw new Error(`OOP counter case ${caseIndex + 1} field ${field + 1} differs: production ${actual}, Turbo Pascal ${fields[field]}`);
      }
    }
  }

  console.log('Turbo Pascal versus production OOP #GIVE/#TAKE: all 24 signed-boundary counter values and same-line command-flow results match.');
})();
