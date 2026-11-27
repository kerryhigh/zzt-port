import fs from 'node:fs';

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
const integratedWasmPath = process.argv[4];
if (!pascalPath || !wasmPath || !integratedWasmPath) {
  throw new Error('Usage: compare-board-edge.js <BOARDENT.OUT> <board_edge.wasm> <integrated_gameplay.wasm>');
}

const labels = ['edge-north', 'edge-south', 'edge-west', 'edge-east', 'edge-blocked', 'edge-existing-player'];
const fieldNames = [
  'current board', 'player x', 'player y', 'tile at player', 'input delta x', 'input delta y',
  'board start x', 'board start y', 'board time seconds', 'paused',
];
const text = fs.readFileSync(pascalPath, 'ascii').trim();
const lines = text.split(/\r?\n/);
if (lines.length !== labels.length) {
  throw new Error(`Pascal BoardEnter replay emitted ${lines.length} rows; expected ${labels.length}`);
}

const expected = lines.map((line, index) => {
  const match = line.trim().match(/^([^ ]+) board (-?\d+) player (-?\d+) (-?\d+) tile (-?\d+) delta (-?\d+) (-?\d+) start (-?\d+) (-?\d+) time (-?\d+) paused (-?\d+)$/);
  if (!match || match[1] !== labels[index]) {
    throw new Error(`Malformed Pascal board-edge row ${index + 1}: ${line}`);
  }
  return match.slice(2).map(Number);
});

const bytes = fs.readFileSync(wasmPath);
const instance = new WebAssembly.Instance(new WebAssembly.Module(bytes));
const exports = instance.exports;
const result = exports.run_board_edge_compat_checks();
if (result !== 0) throw new Error(`Production board-edge replay returned ${result}`);

const rows = exports.board_edge_output_rows();
const columns = exports.board_edge_output_columns();
if (rows !== labels.length || columns !== fieldNames.length) {
  throw new Error(`Production board-edge replay emitted ${rows}x${columns}; expected ${labels.length}x${fieldNames.length}`);
}
const actual = new Int32Array(exports.memory.buffer, exports.board_edge_output_ptr(), rows * columns);
let callbackCompared = 0;
for (let row = 0; row < rows; row++) {
  for (let column = 0; column < columns; column++) {
    const want = expected[row][column];
    const got = actual[row * columns + column];
    callbackCompared++;
    if (want !== got) {
      throw new Error(`Board-edge callback first difference at ${labels[row]}, ${fieldNames[column]}: Pascal=${want}, production=${got}`);
    }
  }
}

let heapTop = 8 * 1024 * 1024;
const allocations = new Map();
let integratedInstance;
const malloc = size => {
  const length = Math.max(1, Number(size) >>> 0);
  const pointer = heapTop;
  heapTop = (heapTop + length + 7) & ~7;
  if (heapTop > 15 * 1024 * 1024) return 0;
  allocations.set(pointer, length);
  return pointer;
};
const env = new Proxy({}, {
  get: (_target, name) => {
    if (name === 'malloc') return malloc;
    if (name === 'free') return pointer => { allocations.delete(Number(pointer)); };
    if (name === 'realloc') return (pointer, size) => {
      pointer = Number(pointer);
      const length = Math.max(1, Number(size) >>> 0);
      if (!pointer) return malloc(length);
      const oldLength = allocations.get(pointer) || 0;
      if (length <= oldLength) {
        allocations.set(pointer, length);
        return pointer;
      }
      const nextPointer = malloc(length);
      if (!nextPointer) return 0;
      if (integratedInstance) {
        new Uint8Array(integratedInstance.exports.memory.buffer)
          .copyWithin(nextPointer, pointer, pointer + oldLength);
      }
      return nextPointer;
    };
    return () => 0;
  }
});
integratedInstance = await WebAssembly.instantiate(
  new WebAssembly.Module(fs.readFileSync(integratedWasmPath)), { env });
const integratedExports = integratedInstance.exports;
const fullResult = integratedExports.run_integrated_board_edge_actual_transition_checks();
if (fullResult !== 0) throw new Error(`Full production board-edge transition replay returned ${fullResult}`);
const fullRows = integratedExports.integrated_board_edge_actual_output_rows();
const fullColumns = integratedExports.integrated_board_edge_actual_output_columns();
if (fullRows !== labels.length || fullColumns !== fieldNames.length) {
  throw new Error(`Full production board-edge replay emitted ${fullRows}x${fullColumns}; expected ${labels.length}x${fieldNames.length}`);
}
const fullActual = new Int32Array(integratedExports.memory.buffer,
  integratedExports.integrated_board_edge_actual_output_ptr(), fullRows * fullColumns);
let fullCompared = 0;
for (let row = 0; row < fullRows; row++) {
  for (let column = 0; column < fullColumns; column++) {
    const want = expected[row][column];
    const got = fullActual[row * fullColumns + column];
    fullCompared++;
    if (want !== got) {
      throw new Error(`Full production board-edge first difference at ${labels[row]}, ${fieldNames[column]}: Pascal=${want}, production=${got}`);
    }
  }
}

console.log(`Board-edge callback-boundary Pascal/production differential: ${rows} routes, ${callbackCompared} fields matched under controlled service stubs.`);
console.log(`Board-edge full production Pascal differential: ${rows} routes, ${fullCompared} fields matched through actual BoardChange, serialized board open/close, call-stack continuation, and BoardEnter.`);
