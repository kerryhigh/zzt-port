const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-blink-wall-tick.js BLINKPRB.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedLabels = ['right-empty', 'left-empty'];
if (rows.length !== expectedLabels.length) {
  throw new Error(`Turbo Pascal Blink Wall output should contain 2 cases; got ${rows.length}`);
}

const pascalValues = [];
for (let i = 0; i < rows.length; i++) {
  const match = rows[i].match(/^([a-z-]+) player (-?\d+) (-?\d+) origin (-?\d+) (-?\d+) right (-?\d+) (-?\d+) left (-?\d+) (-?\d+) player-under (-?\d+) (-?\d+) p3 (-?\d+) health (-?\d+) stats (-?\d+) timer (-?\d+) message (.*)$/);
  if (!match) throw new Error(`Malformed Turbo Pascal Blink Wall case ${i + 1}: ${rows[i]}`);
  if (match[1] !== expectedLabels[i]) {
    throw new Error(`Unexpected Turbo Pascal Blink Wall case ${i + 1}: ${match[1]}`);
  }
  const message = [...match[16]].map((character) => character.charCodeAt(0));
  if (message.length > 5) throw new Error(`Unexpectedly long Blink Wall message: ${match[16]}`);
  const paddedMessage = [...message, ...Array(5 - message.length).fill(-1)];
  pascalValues.push(
    i + 1,
    Number(match[2]), Number(match[3]),
    Number(match[13]), Number(match[14]), Number(match[15]),
    message.length, ...paddedMessage,
    Number(match[10]), Number(match[11]),
    Number(match[4]), Number(match[5]),
    Number(match[6]), Number(match[7]),
    Number(match[8]), Number(match[9]),
    Number(match[12]),
  );
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_blink_wall_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Blink Wall tick capture returned ${status}`);

  const pointer = instance.exports.blink_wall_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 2 * 21);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 21);
      const field = i % 21;
      throw new Error(`Blink Wall case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Blink Wall tick: both push directions, damage/message effects, tile colors, under-tile state, timer, and the left-check/right-move quirk match across two cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
