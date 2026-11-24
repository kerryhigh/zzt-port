const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-flow.js OOPFLOW.OUT oop_label.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedCaseCount = 11;
const fieldsPerCase = 11;
if (rows.length !== expectedCaseCount) {
  throw new Error(`Turbo Pascal OOP flow probe should contain ${expectedCaseCount} cases; got ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_oop_flow_capture_checks();
  if (status !== 0) throw new Error(`Production OOP flow capture returned ${status}`);

  const pointer = instance.exports.oop_flow_output_ptr();
  const view = new DataView(instance.exports.memory.buffer);
  for (let caseIndex = 0; caseIndex < expectedCaseCount; caseIndex++) {
    const fields = rows[caseIndex].trim().split(/\s+/).map(Number);
    if (fields.length !== fieldsPerCase || fields.some((value) => !Number.isInteger(value))) {
      throw new Error(`Malformed Turbo Pascal OOP flow case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    if (fields[0] !== caseIndex + 1) {
      throw new Error(`Turbo Pascal OOP flow output is out of order at case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    const actualFields = [];
    for (let field = 0; field < fieldsPerCase; field++) {
      const actual = view.getInt16(pointer + (caseIndex * fieldsPerCase + field) * 2, true);
      actualFields.push(actual);
    }
    for (let field = 0; field < fieldsPerCase; field++) {
      if (actualFields[field] !== fields[field]) {
        throw new Error(`OOP flow case ${caseIndex + 1} field ${field + 1} differs: production ${actualFields[field]}, Turbo Pascal ${fields[field]}; production row ${actualFields.join(' ')}, Pascal row ${fields.join(' ')}`);
      }
    }
  }

  console.log('Turbo Pascal versus production OOP flow: IDLE, END, ENDGAME, LOCK/UNLOCK, RESTART, blocked/open GO, and blocked/open TRY match across 11 snapshots.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
