const fs = require('fs');

const [pascalOutputPath, productionWasmPath] = process.argv.slice(2);
if (!pascalOutputPath || !productionWasmPath) {
  throw new Error('Usage: node compare-input-buffer.js INPUTBUF.OUT player_input.wasm');
}

const pascalRows = fs.readFileSync(pascalOutputPath, 'utf8')
  .split(/\r?\n/)
  .filter((line) => line.startsWith('arrow-right-burst '));
if (pascalRows.length !== 3) {
  throw new Error(`Expected three Pascal arrow-burst rows, found ${pascalRows.length}`);
}
const expected = pascalRows.flatMap((line) => {
  const fields = line.trim().split(/\s+/).slice(1, 4).map(Number);
  if (fields.length !== 3 || fields.some((value) => !Number.isFinite(value))) {
    throw new Error(`Malformed Pascal arrow-burst row: ${line}`);
  }
  return fields;
});
const pascalLines = fs.readFileSync(pascalOutputPath, 'utf8').split(/\r?\n/);
const keyBurstCases = [
  ['one', 2],
  ['two', 3],
  ['three', 3],
  ['four', 3],
  ['repeat-four', 3]
];
const expectedKeyBursts = keyBurstCases.flatMap(([label, rowCount]) => {
  const rows = pascalLines.filter((line) => line.startsWith(`${label} `));
  if (rows.length !== rowCount) {
    throw new Error(`Expected ${rowCount} Pascal ${label} rows, found ${rows.length}`);
  }
  return rows.map((line) => {
    const fields = line.trim().split(/\s+/);
    const value = Number(fields[1]);
    if (fields.length !== 2 || fields[0] !== label || !Number.isInteger(value)) {
      throw new Error(`Malformed Pascal ${label} row: ${line}`);
    }
    return value;
  });
});
const cp437Rows = fs.readFileSync(pascalOutputPath, 'utf8')
  .split(/\r?\n/)
  .filter((line) => line.startsWith('cp437-'));
if (cp437Rows.length !== 2) {
  throw new Error(`Expected two Pascal CP437 input rows, found ${cp437Rows.length}`);
}
const expectedHighBytes = cp437Rows.map((line, index) => {
  const fields = line.trim().split(/\s+/);
  const expectedLabel = index === 0 ? 'cp437-high' : 'cp437-top';
  if (fields.length !== 2 || fields[0] !== expectedLabel
      || !Number.isInteger(Number(fields[1]))) {
    throw new Error(`Malformed Pascal CP437 input row: ${line}`);
  }
  return Number(fields[1]);
});

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(productionWasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const result = instance.exports.run_player_input_checks();
  if (result !== 0) throw new Error(`Production player-input regression returned ${result}`);
  const outputPointer = instance.exports.player_input_arrow_burst_output_ptr();
  const actual = Array.from(new Int16Array(instance.exports.memory.buffer, outputPointer, 11));
  const keyBurstPointer = instance.exports.player_input_key_burst_output_ptr();
  const actualKeyBursts = Array.from(new Int16Array(instance.exports.memory.buffer, keyBurstPointer, 14));
  if (actualKeyBursts.length !== expectedKeyBursts.length) {
    throw new Error(`Pascal/production key-burst counts differ: Pascal=${expectedKeyBursts.length}, production=${actualKeyBursts.length}`);
  }
  for (let i = 0; i < expectedKeyBursts.length; i++) {
    if (actualKeyBursts[i] !== expectedKeyBursts[i]) {
      throw new Error(`Queued key-burst field ${i} differs: Pascal=${expectedKeyBursts[i]}, production=${actualKeyBursts[i]}`);
    }
  }
  for (let i = 0; i < expected.length; i++) {
    if (actual[i] !== expected[i]) {
      throw new Error(`Queued arrow input field ${i} differs: Pascal=${expected[i]}, production=${actual[i]}`);
    }
  }
  for (let i = 0; i < expectedHighBytes.length; i++) {
    if (actual[9 + i] !== expectedHighBytes[i]) {
      throw new Error(`CP437 input byte ${i} differs: Pascal=${expectedHighBytes[i]}, production=${actual[9 + i]}`);
    }
  }
  console.log('Turbo Pascal versus production input: one-to-four-key bursts, repeated-key bursts, arrow movement bursts, and CP437 bytes 0x82/0xFF match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
