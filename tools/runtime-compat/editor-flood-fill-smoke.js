const fs = require('node:fs');

const [wasmPath] = process.argv.slice(2);
if (!wasmPath) throw new Error('Usage: node editor-flood-fill-smoke.js editor_flood_fill.wasm');

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const result = instance.exports.run_editor_fill_self_checks();
  if (result !== 0) throw new Error(`Extracted editorFloodFill production self-check failed: ${result}`);
  const placementResult = instance.exports.run_editor_placement_checks();
  if (placementResult !== 0) {
    throw new Error(`Extracted editor placement/paste production self-check failed: ${placementResult}`);
  }
  const placementPointer = instance.exports.editor_placement_output_ptr();
  const placement = Array.from(new Uint8Array(instance.exports.memory.buffer, placementPointer, 20));
  const expectedPlacement = [
    0, 22, 14, 1, 0,
    1, 4, 15, 1, 0,
    0, 19, 127, 1, 0,
    1, 21, 15, 0, 150,
  ];
  if (placement.some((value, index) => value !== expectedPlacement[index])) {
    throw new Error(`Editor placement production state differs: ${placement.join(',')}`);
  }
  console.log('Extracted Android editor helpers: flood-fill, pattern/player/tile paste, MAX_STAT refusal, and shallow script-bearing Bomb copy/paste self-check pass.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
