const fs = require('fs');

const [pascalOutputPath, productionWasmPath, pascalMatrixPath] = process.argv.slice(2);
if (!pascalOutputPath || !productionWasmPath || !pascalMatrixPath) {
  throw new Error('Usage: node compare-input-buffer.js INPUTBUF.OUT player_input.wasm INPUTMAT.OUT');
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
const wrappedRows = pascalLines.filter((line) => line.startsWith('wrap-burst '));
if (wrappedRows.length !== 2) {
  throw new Error(`Expected two Pascal ring-wrap burst rows, found ${wrappedRows.length}`);
}
const expectedWrapped = wrappedRows.flatMap((line) => {
  const fields = line.trim().split(/\s+/).slice(1).map(Number);
  if (fields.length !== 5 || fields.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Pascal ring-wrap row: ${line}`);
  }
  return fields;
});

const inputMatrixLines = fs.readFileSync(pascalMatrixPath, 'utf8')
  .split(/\r?\n/)
  .filter((line) => line.startsWith('matrix '));
const matrixRows = 256 * 2;
const matrixFields = 7;
if (inputMatrixLines.length !== matrixRows) {
  throw new Error(`Expected ${matrixRows} Pascal input-length rows, found ${inputMatrixLines.length}`);
}
const expectedMatrix = inputMatrixLines.flatMap((line, index) => {
  const fields = line.trim().split(/\s+/);
  const values = fields.slice(1).map(Number);
  const expectedLength = Math.floor(index / 2);
  const expectedStage = (index % 2) + 1;
  if (fields.length !== matrixFields + 1 || fields[0] !== 'matrix'
      || values.some((value) => !Number.isInteger(value))
      || values[0] !== expectedLength || values[1] !== expectedStage) {
    throw new Error(`Malformed Pascal input-length row ${index}: ${line}`);
  }
  return values;
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
  const wrappedPointer = instance.exports.player_input_wrapped_burst_output_ptr();
  const actualWrapped = Array.from(new Int16Array(instance.exports.memory.buffer, wrappedPointer, expectedWrapped.length));
  for (let i = 0; i < expectedWrapped.length; i++) {
    if (actualWrapped[i] !== expectedWrapped[i]) {
      throw new Error(`Wrapped input queue field ${i} differs: Pascal=${expectedWrapped[i]}, production=${actualWrapped[i]}`);
    }
  }
  const matrixStatus = instance.exports.run_input_length_matrix_checks();
  if (matrixStatus !== 0) throw new Error(`Production input-length matrix returned ${matrixStatus}`);
  const matrixPointer = instance.exports.player_input_length_matrix_output_ptr();
  const actualMatrix = Array.from(new Int16Array(
    instance.exports.memory.buffer, matrixPointer, matrixRows * matrixFields));
  for (let i = 0; i < expectedMatrix.length; i++) {
    if (actualMatrix[i] !== expectedMatrix[i]) {
      const row = Math.floor(i / matrixFields);
      const field = i % matrixFields;
      const fieldNames = ['length', 'stage', 'key_pressed', 'buffer_count', 'residual_byte', 'delta_x', 'delta_y'];
      throw new Error(`Input queue first divergence at length ${Math.floor(row / 2)}, stage ${(row % 2) + 1}, ${fieldNames[field]}: Pascal=${expectedMatrix[i]}, production=${actualMatrix[i]}`);
    }
  }
  console.log('Turbo Pascal versus production input: key bursts, ring wrap, and CP437 bytes match; all 256 buffer lengths (0..255), 512 Update transitions and residual buffers match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
