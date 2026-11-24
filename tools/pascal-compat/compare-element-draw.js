const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-element-draw.js DRAWPRB.OUT oop_movement.wasm');
}

const starChars = [179, 47, 196, 92];
const transporterChars = [
  [94, 126, 94, 45],
  [118, 95, 118, 45],
  [40, 60, 40, 179],
  [41, 62, 41, 179],
];
const expectedRows = [];
const pascalValues = [];
for (const [label, startColor, phases] of [['15', 15, 4], ['14', 14, 4], ['255', 255, 1]]) {
  let color = startColor;
  for (let phase = 0; phase < phases; phase++) {
    color = (color + 1) & 0xff;
    if (color > 15) color = 9;
    const character = starChars[phase];
    expectedRows.push(`star-${label} ${phase} ${character} ${color}`);
    pascalValues.push(character, color);
  }
}
for (let direction = 0; direction < transporterChars.length; direction++) {
  for (let phase = 0; phase < 4; phase++) {
    const character = transporterChars[direction][phase];
    expectedRows.push(`transporter ${direction} ${phase} ${character}`);
    pascalValues.push(character);
  }
}
for (let direction = 0; direction < transporterChars.length; direction++) {
  for (let phase = 0; phase < 4; phase++) {
    const character = transporterChars[direction][phase];
    expectedRows.push(`transporter-tick ${direction} ${phase} 1 9 9 15 ${character}`);
    pascalValues.push(direction, phase, 1, 9, 9, 15, character);
  }
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== expectedRows.length) {
  throw new Error(`Turbo Pascal element draw output should contain ${expectedRows.length} rows; got ${rows.length}`);
}
for (let i = 0; i < rows.length; i++) {
  if (rows[i] !== expectedRows[i]) {
    throw new Error(`Turbo Pascal element draw row ${i + 1} differs: got "${rows[i]}", expected "${expectedRows[i]}"`);
  }
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_element_draw_capture_checks();
  if (status !== 0) throw new Error(`Production element draw capture returned ${status}`);

  const pointer = instance.exports.element_draw_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, pascalValues.length);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      throw new Error(`Element draw value ${i + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production element drawing: Star glyph/color frames, Transporter directions, and all 16 Transporter tick redraws match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
