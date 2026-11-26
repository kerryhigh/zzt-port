const fs = require('node:fs');

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: compare-missing-world-load.js <MISSLOAD.OUT> <world_reload.wasm>');
}

const fieldNames = [
  'WorldLoad result / production error', 'ReturnBoardId', 'World.BoardCount',
  'World.Info.CurrentBoard', 'World.Info.IsSave', 'World.Info.Ammo', 'World.Info.Gems',
  'World.Info.Health', 'World.Info.Torches', 'World.Info.TorchTicks',
  'World.Info.EnergizerTicks', 'World.Info.Score', 'Board.StatCount',
  'Board.Stats[0].X', 'Board.Stats[0].Y', 'Board.Stats[0].Cycle',
  'Board.Tiles[10][12].Element', 'Board.Tiles[10][12].Color', 'selected world filename',
];

function context(expected, actual, index) {
  const start = Math.max(0, index - 2);
  const end = Math.min(fieldNames.length - 1, index + 2);
  return Array.from({ length: end - start + 1 }, (_, offset) => {
    const field = start + offset;
    return { field: fieldNames[field], pascal: expected[field], production: actual[field] };
  });
}

(async () => {
  const expected = fs.readFileSync(pascalPath, 'ascii').trim().split(/\s+/).map(Number);
  if (expected.length !== fieldNames.length || expected.some(value => !Number.isInteger(value))) {
    throw new Error(`Malformed Pascal missing-world output: ${expected.length} fields; expected ${fieldNames.length}.`);
  }
  if (expected[0] !== 0) throw new Error(`Pascal WorldLoad unexpectedly succeeded (${expected[0]}).`);
  if (expected[18] !== 1) throw new Error('Pascal probe did not retain the selected MISSFILE world name.');

  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  // Imports reachable only from zoo_world_play's successful-load branch are
  // inert here; the fixture requires the missing-file error path to return.
  const env = new Proxy({}, { get: () => () => 0 });
  const instance = await WebAssembly.instantiate(module, { env });
  const result = instance.exports.run_world_play_missing_reload_capture();
  if (result !== 0) throw new Error(`Production missing-world reload harness returned ${result}.`);
  const count = instance.exports.world_reload_missing_output_count();
  if (count !== fieldNames.length) throw new Error(`Production emitted ${count} fields; expected ${fieldNames.length}.`);
  const pointer = instance.exports.world_reload_missing_output_ptr();
  const actual = Array.from(new Int32Array(instance.exports.memory.buffer, pointer, count));
  if (actual[0] !== -2) throw new Error(`Missing production file returned ${actual[0]}; expected ZOO_ERROR_INVAL (-2).`);
  if (actual[18] !== 1) throw new Error('Production did not open the selected MISSFILE.ZZT path.');

  for (let index = 1; index < fieldNames.length; index++) {
    if (expected[index] !== actual[index]) {
      throw new Error(`First missing-world reload divergence at field ${index} (${fieldNames[index]}): Pascal=${expected[index]}, production=${actual[index]}; nearby=${JSON.stringify(context(expected, actual, index))}`);
    }
  }
  console.log(`Missing title-P reload path matched: Pascal WorldLoad returned false, LibZoo zoo_world_play returned ZOO_ERROR_INVAL (-2) without starting play, and all ${fieldNames.length - 1} preserved world/board fields matched.`);
})().catch(error => { console.error(error); process.exitCode = 1; });
