const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-message-timer.js MSGTIK.OUT oop_movement.wasm');
}

const caseNames = [
  'hold', 'expire-current', 'expire-before', 'nonzero-x', 'byte-wrap', 'zero-x-side-y', 'zero-wrap'
];
const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== caseNames.length) {
  throw new Error(`Turbo Pascal message-timer output should contain ${caseNames.length} cases; got ${rows.length}`);
}

const pascalValues = [];
for (let i = 0; i < caseNames.length; i++) {
  const parts = rows[i].trim().split(/\s+/);
  const name = parts.shift();
  const values = parts.map(Number);
  if (name !== caseNames[i] || values.length !== 138 || values.some((value) => !Number.isInteger(value))) {
    throw new Error(`Malformed Turbo Pascal message-timer case ${i + 1}: ${rows[i]}`);
  }
  const expectedHeaders = [
    [1, 1, 2, 2],
    [0, 0, 0, -1],
    [1, 0, 0, -1],
    [1, 1, 4, 2],
    [1, 1, 4, 254],
    [1, 1, 4, 1],
    [1, 1, 4, 255],
  ][i];
  if (values.slice(0, 4).some((value, index) => value !== expectedHeaders[index])) {
    throw new Error(`Unexpected Turbo Pascal message-timer header in case ${caseNames[i]}: ${values.slice(0, 4).join(' ')}`);
  }
  pascalValues.push(...values);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_message_timer_capture_checks();
  if (status !== 0) throw new Error(`Production message-timer capture returned ${status}`);

  const pointer = instance.exports.message_timer_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, pascalValues.length);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 138);
      const field = i % 138;
      throw new Error(`Message-timer case ${caseNames[caseIndex]} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production message timer: message rendering, P2 byte decrement wrap, x-coordinate gate, stat cursor removal quirk, and final border redraw match across seven cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
