const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-rng-vector.js RNGVEC.OUT rng.wasm');
}

const seeds = [
  0, 1, 123, 2147483647, -2147483648, -1, -2147483647,
  330, 26, 3, 85, 256, 454, -596792289, -1860696767, 134775814
];
const ranges = [0, 1, 2, 3, 4, 7, 9, 10, 17, 100, 440, 1600, 5000, 32767];
const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedRows = seeds.length * ranges.length;
if (rows.length !== expectedRows) {
  throw new Error(`Turbo Pascal RNG vector should contain ${expectedRows} rows; got ${rows.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const api = instance.exports;
  let rowIndex = 0;
  for (let seedIndex = 0; seedIndex < seeds.length; seedIndex++) {
    api.rng_vector_set_seed(seeds[seedIndex] >>> 0);
    for (let rangeIndex = 0; rangeIndex < ranges.length; rangeIndex++, rowIndex++) {
      const fields = rows[rowIndex].trim().split(/\s+/).map(Number);
      if (fields.length !== 6) {
        throw new Error(`Malformed Turbo Pascal RNG row ${rowIndex + 1}: ${rows[rowIndex]}`);
      }
      const [actualSeedIndex, actualRangeIndex, actualRange, pascalValue, pascalLow, pascalHigh] = fields;
      const expectedRow = [seedIndex + 1, rangeIndex + 1, ranges[rangeIndex]];
      if (actualSeedIndex !== expectedRow[0] || actualRangeIndex !== expectedRow[1]
          || actualRange !== expectedRow[2]) {
        throw new Error(`Turbo Pascal RNG row ${rowIndex + 1} has wrong seed/range identity: ${rows[rowIndex]}`);
      }
      const productionValue = api.rng_vector_next(ranges[rangeIndex]);
      const productionLow = api.rng_vector_seed_low();
      const productionHigh = api.rng_vector_seed_high();
      const actual = [pascalValue, pascalLow, pascalHigh];
      const expected = [productionValue, productionLow, productionHigh];
      for (let field = 0; field < actual.length; field++) {
        if (actual[field] !== expected[field]) {
          throw new Error(`RNG mismatch at seed ${seeds[seedIndex]}, draw ${rangeIndex + 1}, `
            + `Random(${ranges[rangeIndex]}): field ${field + 1} Pascal=${actual[field]}, `
            + `production=${expected[field]}; row=${rows[rowIndex]}`);
        }
      }
    }
  }
  console.log(`Turbo Pascal RNG vector: ${expectedRows} seeded draws matched results and 32-bit state words (D/DP).`);
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
