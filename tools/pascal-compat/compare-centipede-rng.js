const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-centipede-rng.js CENTIRNG.OUT oop_movement.wasm');
}

const names = ['x-aligned', 'y-aligned'];
const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== names.length) {
  throw new Error(`Turbo Pascal Centipede RNG output should have ${names.length} rows; got ${rows.length}`);
}

const expected = [];
for (let i = 0; i < names.length; i++) {
  const match = rows[i].match(/^(x-aligned|y-aligned)(?:\s+-?\d+){17}$/);
  if (!match || match[1] !== names[i]) {
    throw new Error(`Malformed Turbo Pascal Centipede RNG row ${i + 1}: ${rows[i]}`);
  }
  const fields = rows[i].split(/\s+/).slice(1).map(Number);
  expected.push(...fields);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_centipede_rng_caller_checks();
  if (status !== 0) throw new Error(`Production Centipede RNG capture returned ${status}`);

  const pointer = instance.exports.centipede_rng_output_ptr();
  const actual = new Int32Array(instance.exports.memory.buffer, pointer, names.length * 17);
  for (let i = 0; i < expected.length; i++) {
    if (actual[i] !== expected[i]) {
      const caseIndex = Math.floor(i / 17);
      const field = i % 17;
      throw new Error(`Centipede RNG case ${names[caseIndex]}, field ${field + 1} differs: production ${actual[i]}, Turbo Pascal ${expected[i]}; preceding fields=${JSON.stringify(Array.from(actual.slice(Math.max(0, i - 3), i + 4)))}`);
    }
  }

  console.log('Turbo Pascal versus production Centipede aligned-axis callers: both Random(10) branches, value, final seed, movement, and source/destination cells match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
