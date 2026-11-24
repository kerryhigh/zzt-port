const fs = require('node:fs');

const [pascalPath, pascalStatPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !pascalStatPath || !wasmPath) {
  throw new Error('Usage: node compare-editor-placement.js EDPLACE.OUT EDSTAT.OUT editor_flood_fill.wasm');
}

const placementLayouts = [
  {label: 'N', result: 0, fieldCount: 4},
  {label: 'P', result: 1, fieldCount: 4},
  {label: 'C', result: 0, fieldCount: 4},
  {label: 'L', result: 1, fieldCount: 5},
];
const statLayouts = [
  {label: 'S', result: 0, fieldCount: 24},
  {label: 'D', result: 0, fieldCount: 24},
];
function readRows(path, layouts, name) {
  const lines = fs.readFileSync(path, 'ascii').trim().split(/\r?\n/);
  if (lines.length !== layouts.length) {
    throw new Error(`Expected ${layouts.length} Pascal ${name} rows, found ${lines.length}`);
  }
  return lines.map((line, index) => {
    const fields = line.trim().split(/\s+/);
    const layout = layouts[index];
    if (fields.length !== layout.fieldCount || fields[0] !== layout.label) {
      throw new Error(`Malformed Pascal ${name} row ${index + 1}: ${line}`);
    }
    return [layout.result, ...fields.slice(1).map(Number)];
  });
}
const pascalPlacement = readRows(pascalPath, placementLayouts, 'editor-placement');
const pascalStat = readRows(pascalStatPath, statLayouts, 'stat-paste');

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const {exports} = await WebAssembly.instantiate(module, {
    env: new Proxy({}, {get: () => () => 0})
  });
  const result = exports.run_editor_placement_checks();
  if (result !== 0) throw new Error(`Production editor-placement self-check returned ${result}`);
  const pointer = exports.editor_placement_output_ptr();
  const production = Array.from(new Uint8Array(exports.memory.buffer, pointer, 68));
  for (let row = 0; row < placementLayouts.length; row++) {
    for (let field = 0; field < pascalPlacement[row].length; field++) {
      const actual = production[row * 5 + field];
      if (actual !== pascalPlacement[row][field]) {
        throw new Error(`Editor ${placementLayouts[row].label} row field ${field + 1} differs: Pascal ${pascalPlacement[row][field]}, production ${actual}`);
      }
    }
  }
  for (let row = 0; row < statLayouts.length; row++) {
    for (let field = 0; field < pascalStat[row].length; field++) {
      const actual = production[20 + row * 24 + field];
      if (actual !== pascalStat[row][field]) {
        throw new Error(`Editor ${statLayouts[row].label} row field ${field + 1} differs: Pascal ${pascalStat[row][field]}, production ${actual}`);
      }
    }
  }
  console.log('Turbo Pascal versus extracted Android editor helpers: pattern placement, protected player, plain tile paste, MAX_STAT refusal, and script-bearing Bomb copy/paste including Pascal shallow-data aliasing match; production no-script paste self-check also passes.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
