const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-send.js OOPSEND.OUT oop_label.wasm');
}

function parseRows() {
  const lines = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
  const layouts = [
    ['S', 2],
    ['E', 2],
    ['X', 3],
    ['D', 1],
    ['Q', 5],
  ];
  if (lines.length !== layouts.length) {
    throw new Error(`Expected ${layouts.length} Pascal OOP SEND rows, found ${lines.length}`);
  }
  return lines.map((line, index) => {
    const fields = line.trim().split(/\s+/);
    const [prefix, valueCount] = layouts[index];
    if (fields.length !== valueCount + 1 || fields[0] !== prefix) {
      throw new Error(`Malformed Pascal OOP SEND row ${index + 1}: ${line}`);
    }
    return fields.slice(1).map(Number);
  });
}

function readSigned16(bytes, offset) {
  const value = bytes[offset] | (bytes[offset + 1] << 8);
  return value & 0x8000 ? value - 0x10000 : value;
}

(async () => {
  const wasm = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const { exports } = await WebAssembly.instantiate(wasm, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const result = exports.run_oop_send_capture_checks();
  if (result !== 0) throw new Error(`Production OOP SEND capture returned ${result}`);

  const pointer = exports.oop_send_capture_output_ptr();
  const actual = new Uint8Array(exports.memory.buffer, pointer, 23);
  const production = [
    [actual[0], readSigned16(actual, 1)],
    [actual[3], readSigned16(actual, 4)],
    [actual[6], readSigned16(actual, 7), readSigned16(actual, 9)],
    [readSigned16(actual, 11)],
    [readSigned16(actual, 13), readSigned16(actual, 15),
      readSigned16(actual, 17), readSigned16(actual, 19), readSigned16(actual, 21)],
  ];
  const pascal = parseRows();
  const expected = [
    [1, 7], // Positive object self-send ignores the object's own lock.
    [0, 0], // Negative event send respects the locked receiver.
    [0, 0, 8], // A locked duplicate is skipped; the later name match receives the label.
    [17], // Parsed self-send resumes at GO before the following same-line CYCLE.
    [5, 0, 8, -1, 9], // Cross-send skips the lock, targets byte 8, then #END stores -1.
  ];

  for (let row = 0; row < expected.length; row++) {
    for (let column = 0; column < expected[row].length; column++) {
      const p = pascal[row][column];
      const c = production[row][column];
      if (p !== expected[row][column] || c !== p) {
        throw new Error(`OOP SEND row ${row + 1}, field ${column + 1} differs: Pascal ${p}, production ${c}, expected ${expected[row][column]}`);
      }
    }
  }
  console.log('Turbo Pascal versus production OOP #SEND: self/event lock rules, duplicate target iteration, same-line self-dispatch, and cross-object recipient execution match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
