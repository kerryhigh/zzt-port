const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-transition-table.js TRNTBL.OUT integrated_gameplay.wasm');
}

const width = 60;
const height = 25;
const cellCount = width * height;
const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== cellCount + 1) {
  throw new Error(`Turbo Pascal transition-table probe should contain ${cellCount + 1} rows; got ${rows.length}`);
}

const expected = [];
for (let index = 0; index < cellCount; index++) {
  const fields = rows[index].trim().split(/\s+/).map(Number);
  if (fields.length !== 3 || fields.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Turbo Pascal transition-table row ${index + 1}: ${rows[index]}`);
  }
  if (fields[0] !== index + 1 || fields[1] < 1 || fields[1] > width || fields[2] < 1 || fields[2] > height) {
    throw new Error(`Invalid Turbo Pascal transition-table entry ${index + 1}: ${rows[index]}`);
  }
  expected.push(fields[1], fields[2]);
}
const seedFields = rows[cellCount].trim().split(/\s+/);
if (seedFields.length !== 3 || seedFields[0] !== 'SEED'
    || !Number.isInteger(Number(seedFields[1])) || !Number.isInteger(Number(seedFields[2]))) {
  throw new Error(`Malformed Turbo Pascal transition-table RNG state: ${rows[cellCount]}`);
}
expected.push(Number(seedFields[1]), Number(seedFields[2]));

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_transition_table_capture_checks();
  if (status !== 0) throw new Error(`Production transition-table capture returned ${status}`);

  const pointer = instance.exports.transition_table_output_ptr();
  const view = new DataView(instance.exports.memory.buffer);
  let firstDifference = -1;
  const actual = [];
  for (let field = 0; field < expected.length; field++) {
    actual.push(view.getUint16(pointer + field * 2, true));
    if (firstDifference < 0 && actual[field] !== expected[field]) firstDifference = field;
  }
  if (firstDifference >= 0) {
    if (firstDifference < cellCount * 2) {
      const cellIndex = Math.floor(firstDifference / 2);
      const firstContext = Math.max(0, cellIndex - 2);
      const lastContext = Math.min(cellCount - 1, cellIndex + 2);
      const context = [];
      for (let index = firstContext; index <= lastContext; index++) {
        const offset = index * 2;
        context.push(`#${index + 1} Pascal=${expected[offset]},${expected[offset + 1]} production=${actual[offset]},${actual[offset + 1]}`);
      }
      const fieldName = firstDifference % 2 === 0 ? 'X' : 'Y';
      throw new Error(`TransitionTable[${cellIndex + 1}].${fieldName} differs: production ${actual[firstDifference]}, Turbo Pascal ${expected[firstDifference]}; surrounding entries: ${context.join('; ')}`);
    }
    const stateField = firstDifference - cellCount * 2;
    throw new Error(`RandSeed.${stateField === 0 ? 'low' : 'high'} differs: production ${actual[firstDifference]}, Turbo Pascal ${expected[firstDifference]}; Pascal seed row ${rows[cellCount]}`);
  }

  console.log(`Turbo Pascal versus production GenerateTransitionTable matched all ${cellCount} shuffled coordinates and both 16-bit RandSeed words (D/DP).`);
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
