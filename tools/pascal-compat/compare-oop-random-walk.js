const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-random-walk.js OOPRND.OUT oop_label.wasm');
}

const expectedCaseCount = 6;
const fieldsPerCase = 11;
const fieldNames = [
  'case', 'position', 'x', 'y', 'stepX', 'stepY', 'dataPos',
  'originElement', 'destinationElement', 'RandSeed.low', 'RandSeed.high'
];
const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== expectedCaseCount) {
  throw new Error(`Turbo Pascal OOP random walk probe should contain ${expectedCaseCount} cases; got ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_oop_random_walk_capture_checks();
  if (status !== 0) throw new Error(`Production OOP random walk capture returned ${status}`);

  const pointer = instance.exports.oop_random_walk_output_ptr();
  const view = new DataView(instance.exports.memory.buffer);
  for (let caseIndex = 0; caseIndex < expectedCaseCount; caseIndex++) {
    const fields = rows[caseIndex].trim().split(/\s+/).map(Number);
    if (fields.length !== fieldsPerCase || fields.some((value) => !Number.isInteger(value))) {
      throw new Error(`Malformed Turbo Pascal OOP random walk case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    if (fields[0] !== caseIndex + 1) {
      throw new Error(`Turbo Pascal OOP random walk output is out of order at case ${caseIndex + 1}: ${rows[caseIndex]}`);
    }
    const actualFields = [];
    for (let field = 0; field < fieldsPerCase; field++) {
      actualFields.push(view.getInt16(pointer + (caseIndex * fieldsPerCase + field) * 2, true));
    }
    for (let field = 0; field < fieldsPerCase; field++) {
      if (actualFields[field] !== fields[field]) {
        throw new Error(`OOP random walk case ${caseIndex + 1} ${fieldNames[field]} differs: production ${actualFields[field]}, Turbo Pascal ${fields[field]}; production row ${actualFields.join(' ')}, Pascal row ${fields.join(' ')}`);
      }
    }
  }

  console.log('Turbo Pascal versus production OOP random directions: RNDNS, RNDNE, and nested RNDP matched both seeded branches, movement state, cursor state, and RNG state across six OopExecute snapshots (D/DP).');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
