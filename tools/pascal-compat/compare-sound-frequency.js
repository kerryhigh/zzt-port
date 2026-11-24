const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-sound-frequency.js SOUNDFRQ.OUT sound_tick.wasm');
}

const pascalText = fs.readFileSync(pascalPath, 'ascii').trim();
const expected = pascalText.split(/\s+/).map(Number);
if (expected.length !== 72 || expected.some((value) => !Number.isInteger(value) || value < 0 || value > 65535)) {
  throw new Error(`Turbo Pascal pitch probe should contain 72 Word frequencies; got ${expected.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const result = instance.exports.run_sound_pitch_table_checks();
  if (result !== 0) throw new Error(`Production sound pitch capture returned ${result}`);

  const pointer = instance.exports.sound_pitch_output_ptr();
  const actual = new Uint16Array(instance.exports.memory.buffer, pointer, expected.length);
  for (let i = 0; i < expected.length; i++) {
    if (actual[i] !== expected[i]) {
      const octave = Math.floor(i / 12) + 1;
      const note = i % 12;
      throw new Error(`Sound frequency differs at octave ${octave}, note ${note}: `
        + `${actual[i]} != Turbo Pascal ${expected[i]}`);
    }
  }

  console.log('Turbo Pascal SoundFreqTable versus production note dispatch: all 72 playable octave/note frequencies match exactly.');
})();
