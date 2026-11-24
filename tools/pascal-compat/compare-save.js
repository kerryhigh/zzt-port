'use strict';
const fs = require('node:fs');
const path = require('node:path');

(async () => {
  const root = process.cwd();
  const sourcePath = path.resolve(root, process.argv[3] || 'third_party/reconstruction-of-zzt/RES/DEMO.ZZT');
  const pascalPath = path.resolve(root, process.argv[2] || 'build/pascal-compat-run/Z/PASOUT.SAV');
  const appPath = path.join(root, 'app/src/main/assets/DEMO.ZZT');
  const wasmPath = path.join(root, 'build/serializer-compatibility/world_roundtrip.wasm');
  const sourceWorld = fs.readFileSync(sourcePath);
  const appWorld = fs.readFileSync(appPath);
  const pascalSave = fs.readFileSync(pascalPath);

  if (!sourceWorld.equals(appWorld)) throw new Error('Pascal and Android DEMO.ZZT fixtures differ');
  if (pascalSave.length !== sourceWorld.length) {
    throw new Error(`Pascal SAV size ${pascalSave.length} differs from source world size ${sourceWorld.length}`);
  }
  let sourceDifferences = 0;
  for (let i = 0; i < sourceWorld.length; i++) {
    if (sourceWorld[i] !== pascalSave[i]) {
      sourceDifferences++;
      if (i !== 264 || sourceWorld[i] !== 0 || pascalSave[i] !== 1) {
        throw new Error(`Unexpected Pascal/source difference at byte ${i}: ${sourceWorld[i]} -> ${pascalSave[i]}`);
      }
    }
  }
  if (sourceDifferences !== 1) throw new Error(`Expected only IsSave byte 264 to differ; found ${sourceDifferences} differences`);

  const wasm = fs.readFileSync(wasmPath);
  const result = await WebAssembly.instantiate(await WebAssembly.compile(wasm), {});
  const instance = result.instance || result;
  const e = instance.exports;
  new Uint8Array(e.memory.buffer, e.input_ptr(), appWorld.length).set(appWorld);
  const rc = e.run_game_save_roundtrip(appWorld.length);
  if (rc !== 0) throw new Error(`zoo_world_save returned ${rc}`);
  const nativeSave = Buffer.from(new Uint8Array(e.memory.buffer, e.output_ptr(), e.output_len()).slice());
  if (!nativeSave.equals(pascalSave)) {
    let at = 0;
    while (at < Math.min(nativeSave.length, pascalSave.length) && nativeSave[at] === pascalSave[at]) at++;
    throw new Error(`Pascal/C SAV mismatch at byte ${at}; sizes ${pascalSave.length}/${nativeSave.length}`);
  }
  console.log(`Turbo Pascal WorldLoad+WorldSave versus source world: only IsSave byte 264 changes; ${pascalSave.length} bytes.`);
  console.log(`Turbo Pascal WorldSave versus production C zoo_world_save: all ${nativeSave.length} SAV bytes identical.`);
})().catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
