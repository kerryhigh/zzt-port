import fs from 'node:fs';

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: compare-board-edge.js <BOARDENT.OUT> <board_edge.wasm>');
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
let compared = 0;
for (let row = 0; row < rows; row++) {
  for (let column = 0; column < columns; column++) {
    const want = expected[row][column];
    const got = actual[row * columns + column];
    compared++;
    if (want !== got) {
      throw new Error(`Board-edge first difference at ${labels[row]}, ${fieldNames[column]}: Pascal=${want}, production=${got}`);
    }
  }
}

console.log(`Board-edge direct Pascal/production differential: ${rows} routes, ${compared} state fields matched (four successful edges, blocked destination rollback, occupied destination player).`);
