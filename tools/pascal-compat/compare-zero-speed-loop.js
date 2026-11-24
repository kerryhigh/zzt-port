const fs = require('node:fs');

const [pascalOutputPath, productionWasmPath] = process.argv.slice(2);
if (!pascalOutputPath || !productionWasmPath) {
  throw new Error('Usage: compare-zero-speed-loop.js ZEROSPD.OUT tick_exit.wasm');
}

const lines = fs.readFileSync(pascalOutputPath, 'ascii').trim().split(/\r?\n/);
if (lines.length !== 1) throw new Error(`Expected one Pascal scheduler row; found ${lines.length}`);
const fields = lines[0].trim().split(/\s+/);
if (fields.length !== 13 || fields[0] !== 'zero-speed'
    || fields[1] !== 'cycles' || fields[3] !== 'delta' || fields[5] !== 'stat'
    || fields[7] !== 'timer' || fields[9] !== 'counter' || fields[11] !== 'exit') {
  throw new Error(`Malformed Pascal zero-speed loop row: ${lines[0]}`);
}
const expected = [Number(fields[2]), Number(fields[4]), Number(fields[6]), Number(fields[8]), Number(fields[10])];
if (Number(fields[12]) !== 1) throw new Error(`Pascal GamePlayLoop did not exit at the test cycle limit: ${lines[0]}`);

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(productionWasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const result = instance.exports.run_zero_speed_schedule_checks();
  if (result !== 0) throw new Error(`Production zero-speed schedule check returned ${result}`);
  const pointer = instance.exports.zero_speed_schedule_output_ptr();
  const actual = new Int16Array(instance.exports.memory.buffer, pointer, expected.length);
  for (let i = 0; i < expected.length; i++) {
    if (actual[i] !== expected[i]) {
      throw new Error(`Zero-speed schedule field ${i + 1} differs: production=${actual[i]}, Pascal=${expected[i]}`);
    }
  }
  console.log('Turbo Pascal GamePlayLoop versus production zero-speed schedule: five immediate cycles, phase, and unchanged clock counters match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
