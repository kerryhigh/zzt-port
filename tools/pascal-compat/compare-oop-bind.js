const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-bind.js BINDSEL.OUT oop_label.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedCaseCount = 5;
if (rows.length !== expectedCaseCount) {
  throw new Error(`Turbo Pascal BIND selector probe should contain ${expectedCaseCount} cases; got ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_oop_bind_selector_checks();
  if (status !== 0) throw new Error(`Production OOP BIND selector harness returned ${status}`);
  const pointer = instance.exports.oop_bind_selector_output_ptr();
  const production = new Uint8Array(instance.exports.memory.buffer, pointer, expectedCaseCount * 5);

  for (let caseIndex = 0; caseIndex < expectedCaseCount; caseIndex++) {
    const fields = rows[caseIndex].trim().split(/\s+/).map(Number);
    if (fields.length !== 5 || fields.some((value) => !Number.isInteger(value))) {
      throw new Error(`Malformed Turbo Pascal BIND selector case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    if (fields[0] !== caseIndex + 1) {
      throw new Error(`Turbo Pascal BIND selector output is out of order at case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    for (let field = 0; field < 5; field++) {
      const actual = production[caseIndex * 5 + field];
      if (actual !== fields[field]) {
        throw new Error(`OOP BIND selector case ${caseIndex + 1} field ${field + 1} differs: production ${actual}, Turbo Pascal ${fields[field]}`);
      }
    }
  }

  console.log('Turbo Pascal versus production OOP #BIND: OTHERS from both first and later stats, ALL, first case-insensitive named match, and missing target match across five cases.');
})();
