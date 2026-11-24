const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-parser.js OOPPARSE.OUT oop_label.wasm');
}

const wordCases = [
  Buffer.from('7MOVE\r', 'ascii'),
  Buffer.from('4ABC\r', 'ascii'),
  Buffer.from('A1\r', 'ascii'),
  Buffer.from('  NORTH\r', 'ascii'),
  Buffer.from('_A:2\r', 'ascii'),
  Buffer.from('ABCDEFGHIJKLMNOPQRSTUV\r', 'ascii'),
  Buffer.alloc(0)
];
for (let value = 0; value < 256; value++) {
  wordCases.push(Buffer.from([value]));
  wordCases.push(Buffer.from([32, value, 65, 49, 33]));
  wordCases.push(Buffer.from([65, value, 66]));
}
const valueCases = [
  Buffer.alloc(0),
  Buffer.from('x', 'ascii'),
  Buffer.from('-9', 'ascii'),
  Buffer.from(' 42,', 'ascii'),
  Buffer.from('32767 ', 'ascii'),
  Buffer.from('32768,', 'ascii'),
  Buffer.from('65535?', 'ascii'),
  Buffer.from('2147483647!', 'ascii'),
  Buffer.from('2147483648!', 'ascii'),
  Buffer.from('2147483649?', 'ascii'),
  Buffer.from('99999999999999999999', 'ascii'),
  Buffer.from('0000000000000000000042#', 'ascii')
];
for (let value = 0; value < 256; value++) {
  valueCases.push(Buffer.from([value]));
  valueCases.push(Buffer.from([32, value, 55, 33]));
}
const lineCases = [];
for (let length = 0; length <= 255; length++) {
  lineCases.push(Buffer.concat([Buffer.alloc(length, 65), Buffer.from([13])]));
}
for (let length = 0; length <= 255; length++) {
  lineCases.push(Buffer.alloc(length, 65));
}
for (let value = 0; value < 256; value++) {
  lineCases.push(Buffer.from([value, 13]));
}
for (let value = 0; value < 256; value++) {
  lineCases.push(Buffer.from([value]));
}
for (let value = 0; value < 256; value++) {
  lineCases.push(Buffer.from([65, value, 66, 13]));
}
for (let value = 0; value < 256; value++) {
  lineCases.push(Buffer.from([65, value, 66]));
}
lineCases.push(Buffer.concat([Buffer.alloc(300, 65), Buffer.from([13])]));
lineCases.push(Buffer.alloc(300, 65));

const lines = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const expectedCount = wordCases.length + valueCases.length + lineCases.length;
if (lines.length !== expectedCount) {
  throw new Error(`Turbo Pascal OOP parser output should contain ${expectedCount} rows; got ${lines.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const memory = instance.exports.memory;
  const inputPointer = instance.exports.oop_value_input_ptr();
  const wordPointer = instance.exports.oop_word_result_ptr();
  const lineInputPointer = instance.exports.oop_line_input_ptr();
  const lineOutputPointer = instance.exports.oop_line_output_ptr();
  let rowIndex = 0;

  const load = (input, pointer = inputPointer, capacity = 64) => {
    if (input.length > capacity) throw new Error(`OOP parser fixture exceeds runtime input capacity: ${input.length}`);
    new Uint8Array(memory.buffer, pointer, input.length).set(input);
  };
  for (let caseIndex = 0; caseIndex < wordCases.length; caseIndex++, rowIndex++) {
    const input = wordCases[caseIndex];
    const fields = lines[rowIndex].trim().split(/\s+/);
    if (fields.length < 4 || fields[0] !== 'W' || Number(fields[1]) !== caseIndex + 1) {
      throw new Error(`Malformed Turbo Pascal OOP word case ${caseIndex + 1}: ${lines[rowIndex]}`);
    }
    const expectedPosition = Number(fields[2]);
    const expectedLength = Number(fields[3]);
    const expectedWord = fields.slice(4).map(Number);
    if (expectedWord.length !== expectedLength) {
      throw new Error(`Turbo Pascal OOP word case ${caseIndex + 1} length mismatch: ${lines[rowIndex]}`);
    }
    load(input);
    const actualPosition = instance.exports.run_oop_word_from_input(input.length);
    const wordMemory = new Uint8Array(memory.buffer, wordPointer, 21);
    const actualLength = Math.min(wordMemory.indexOf(0) < 0 ? wordMemory.length : wordMemory.indexOf(0), 20);
    const actualWord = Array.from(wordMemory.slice(0, actualLength));
    if (actualPosition !== expectedPosition || actualLength !== expectedLength ||
        actualWord.some((byte, index) => byte !== expectedWord[index])) {
      throw new Error(`OOP word case ${caseIndex + 1} differs for ${input.toString('hex')}: ` +
        `production ${actualWord.join(',')}@${actualPosition}, Turbo Pascal ${expectedWord.join(',')}@${expectedPosition}`);
    }
  }

  for (let caseIndex = 0; caseIndex < valueCases.length; caseIndex++, rowIndex++) {
    const input = valueCases[caseIndex];
    const fields = lines[rowIndex].trim().split(/\s+/);
    if (fields.length !== 4 || fields[0] !== 'V' || Number(fields[1]) !== caseIndex + 1) {
      throw new Error(`Malformed Turbo Pascal OOP value case ${caseIndex + 1}: ${lines[rowIndex]}`);
    }
    const expectedValue = Number(fields[2]);
    const expectedPosition = Number(fields[3]);
    load(input);
    const actualValue = instance.exports.run_oop_value_from_input(input.length, 1234);
    const actualPosition = instance.exports.oop_value_result_position();
    if (actualValue !== expectedValue || actualPosition !== expectedPosition) {
      throw new Error(`OOP value case ${caseIndex + 1} differs for ${input.toString('hex')}: ` +
        `production ${actualValue}@${actualPosition}, Turbo Pascal ${expectedValue}@${expectedPosition}`);
    }
  }

  for (let caseIndex = 0; caseIndex < lineCases.length; caseIndex++, rowIndex++) {
    const input = lineCases[caseIndex];
    const fields = lines[rowIndex].trim().split(/\s+/);
    if (fields.length < 4 || fields[0] !== 'L' || Number(fields[1]) !== caseIndex + 1) {
      throw new Error(`Malformed Turbo Pascal OOP line-reader case ${caseIndex + 1}: ${lines[rowIndex]}`);
    }
    const expectedLength = Number(fields[2]);
    const expectedPosition = Number(fields[3]);
    const expectedBytes = fields.slice(4).map(Number);
    if (expectedBytes.length !== expectedLength) {
      throw new Error(`Turbo Pascal OOP line-reader case ${caseIndex + 1} length mismatch: ${lines[rowIndex]}`);
    }
    load(input, lineInputPointer, 320);
    const actualLength = instance.exports.run_oop_line_from_input(input.length);
    const actualPosition = instance.exports.oop_line_result_position();
    const actualBytes = Array.from(new Uint8Array(memory.buffer, lineOutputPointer, actualLength));
    if (actualLength !== expectedLength || actualPosition !== expectedPosition ||
        actualBytes.some((byte, index) => byte !== expectedBytes[index])) {
      throw new Error(`OOP line-reader case ${caseIndex + 1} differs for ${input.toString('hex')}: ` +
        `production ${actualBytes.join(',')}@${actualPosition}, Turbo Pascal ${expectedBytes.join(',')}@${expectedPosition}`);
    }
  }

  console.log(`Turbo Pascal versus production OOP parser: ${wordCases.length} OopReadWord, ${valueCases.length} OopReadValue, and ${lineCases.length} OopReadLineToEnd inputs match, including all byte values, truncation, and cursor positions.`);
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
