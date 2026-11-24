const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-scroll-touch.js SCROLLTH.OUT oop_movement.wasm');
}

const cases = [
  { name: 'default', title: 'Scroll' },
  { name: 'override', title: 'Scroll custom title' },
];
const expectedLines = ['First line', 'Second line'];
const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (rows.length !== cases.length) {
  throw new Error(`Turbo Pascal Scroll touch output should contain ${cases.length} cases; got ${rows.length}`);
}

function parsePascalRow(row, expectedCase) {
  const parts = row.trim().split(/\s+/);
  const name = parts.shift();
  const titleLength = Number(parts.shift());
  const titleBytes = parts.splice(0, titleLength).map(Number);
  const lineCount = Number(parts.shift());
  const lines = [];
  for (let i = 0; i < lineCount; i++) {
    const length = Number(parts.shift());
    lines.push(parts.splice(0, length).map(Number));
  }
  const tail = parts.map(Number);
  const expectedTitle = [...expectedCase.title].map((ch) => ch.charCodeAt(0));
  const expectedLineBytes = expectedLines.map((line) => [...line].map((ch) => ch.charCodeAt(0)));
  if (name !== expectedCase.name || titleLength !== expectedTitle.length
    || titleBytes.length !== expectedTitle.length || titleBytes.some((byte, i) => byte !== expectedTitle[i])
    || lineCount !== expectedLineBytes.length
    || lines.some((line, i) => line.length !== expectedLineBytes[i].length
      || line.some((byte, j) => byte !== expectedLineBytes[i][j]))
    || tail.length !== 5 || tail[0] !== 4 || tail[1] !== 0 || tail[2] !== 0 || tail[3] !== 2 || tail[4] !== -1) {
    throw new Error(`Unexpected Turbo Pascal Scroll touch case ${expectedCase.name}: ${row}`);
  }
  return { titleLength, titleBytes, lineCount, lines };
}

const pascalValues = rows.map((row, i) => parsePascalRow(row, cases[i]));

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_scroll_touch_capture_checks();
  if (status !== 0) throw new Error(`Production Scroll touch capture returned ${status}`);

  const pointer = instance.exports.scroll_touch_output_ptr();
  const production = new Int16Array(instance.exports.memory.buffer, pointer, 2 * 155);
  for (let caseIndex = 0; caseIndex < cases.length; caseIndex++) {
    const base = caseIndex * 155;
    const expected = pascalValues[caseIndex];
    const productionLineCount = production[base];
    const productionTitleLength = production[base + 1];
    if (productionLineCount !== expected.lineCount
      || productionTitleLength !== expected.titleLength) {
      throw new Error(`Scroll touch case ${cases[caseIndex].name} window shape differs: production ${productionLineCount}/${productionTitleLength}, Turbo Pascal ${expected.lineCount}/${expected.titleLength}`);
    }
    for (let i = 0; i < expected.titleBytes.length; i++) {
      if (production[base + 2 + i] !== expected.titleBytes[i]) {
        throw new Error(`Scroll touch case ${cases[caseIndex].name} title byte ${i + 1} differs: production ${production[base + 2 + i]}, Turbo Pascal ${expected.titleBytes[i]}`);
      }
    }
    const productionCapturedLineCount = production[base + 52];
    if (productionCapturedLineCount !== expected.lines.length) {
      throw new Error(`Scroll touch case ${cases[caseIndex].name} captured ${productionCapturedLineCount} lines, Turbo Pascal rendered ${expected.lines.length}`);
    }
    let lineOffset = base + 53;
    for (let lineIndex = 0; lineIndex < expected.lines.length; lineIndex++) {
      const productionLength = production[lineOffset++];
      const expectedLine = expected.lines[lineIndex];
      if (productionLength !== expectedLine.length) {
        throw new Error(`Scroll touch case ${cases[caseIndex].name} line ${lineIndex + 1} length differs: production ${productionLength}, Turbo Pascal ${expectedLine.length}`);
      }
      for (let charIndex = 0; charIndex < expectedLine.length; charIndex++) {
        const actual = production[lineOffset + charIndex];
        if (actual !== expectedLine[charIndex]) {
          throw new Error(`Scroll touch case ${cases[caseIndex].name} line ${lineIndex + 1} byte ${charIndex + 1} differs: production ${actual}, Turbo Pascal ${expectedLine[charIndex]}`);
        }
      }
      lineOffset += 50;
    }
  }

  console.log('Turbo Pascal versus production Scroll touch: both rendered lines, the default Scroll title, and the @ title override match.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
