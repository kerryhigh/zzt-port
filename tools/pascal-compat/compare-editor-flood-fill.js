const fs = require('node:fs');

const [pascalDir, wasmPath] = process.argv.slice(2);
if (!pascalDir || !wasmPath) {
  throw new Error('Usage: node compare-editor-flood-fill.js PASCAL_OUTPUT_DIR editor_flood_fill.wasm');
}

const readPascal = (name) => fs.readFileSync(`${pascalDir}/${name}`, 'ascii').trim().split(/\r?\n/);
const requireHeader = (name, rows, expected) => {
  if (rows[0] !== expected) throw new Error(`${name} expected ${JSON.stringify(expected)}, got ${JSON.stringify(rows[0])}`);
};
const compareRows = (name, expected, actual) => {
  if (expected.length !== actual.length) {
    throw new Error(`${name} row count differs: production ${actual.length}, Turbo Pascal ${expected.length}`);
  }
  for (let row = 0; row < expected.length; row++) {
    if (expected[row] !== actual[row]) {
      throw new Error(`${name} row ${row + 1} differs: production ${actual[row]}, Turbo Pascal ${expected[row]}`);
    }
  }
};

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const exports = instance.exports;
  const snapshotPointer = exports.editor_fill_snapshot_ptr();
  const readBoardRows = () => {
    const snapshot = Buffer.from(new Uint8Array(exports.memory.buffer, snapshotPointer, 60 * 25));
    const text = snapshot.toString('ascii');
    return Array.from({length: 25}, (_, row) => text.slice(row * 60, (row + 1) * 60));
  };
  const run = (caseId) => {
    const changed = exports.run_editor_fill_case(caseId);
    if (changed < 0) throw new Error(`Production editorFloodFill rejected fixture ${caseId}: ${changed}`);
    return {changed, rows: readBoardRows()};
  };

  const enclosed = readPascal('EDITORFL.OUT');
  requireHeader('EDITORFL.OUT', enclosed, 'enclosed-fill 9 center 21 14 outside 21 15');
  const enclosedResult = run(0);
  if (enclosedResult.changed !== 9 ||
      exports.editor_fill_tile_element(30, 12) !== 21 || exports.editor_fill_tile_color(30, 12) !== 14 ||
      exports.editor_fill_tile_element(28, 12) !== 21 || exports.editor_fill_tile_color(28, 12) !== 15) {
    throw new Error('Production editorFloodFill differs from the Turbo Pascal enclosed 3-by-3 result.');
  }

  const wrap = readPascal('EDFLDOVR.OUT');
  requireHeader('EDFLDOVR.OUT', wrap, 'queue-wrap filled 792 empty 0');
  const wrapResult = run(1);
  if (wrapResult.changed !== 792) throw new Error(`Queue-wrap fill count differs: production ${wrapResult.changed}`);
  compareRows('792-cell queue-wrap map', wrap.slice(1), wrapResult.rows);

  const dense = readPascal('EDFLDQ17.OUT');
  requireHeader('EDFLDQ17.OUT', dense, 'dense-queue filled 289 empty 0');
  const denseResult = run(2);
  if (denseResult.changed !== 289) throw new Error(`Dense fill count differs: production ${denseResult.changed}`);
  const denseRows = denseResult.rows.slice(2, 21).map((row) => row.slice(18, 37));
  compareRows('17-by-17 dense queue map with border', dense.slice(1), denseRows);

  const full = readPascal('EDFLDFUL.OUT');
  requireHeader('EDFLDFUL.OUT', full, 'full-board filled 1334 empty 0');
  const fullResult = run(3);
  if (fullResult.changed !== 1334) throw new Error(`Full-interior fill count differs: production ${fullResult.changed}`);
  compareRows('full 60-by-25 board map', full.slice(1), fullResult.rows);

  console.log('Turbo Pascal versus shared LibZoo editor flood fill: enclosed 3-by-3, 792-cell wrap, dense 17-by-17, and full 60-by-25 board maps match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
