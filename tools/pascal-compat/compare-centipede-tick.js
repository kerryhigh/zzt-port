const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-centipede-tick.js CENTIP.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/)
  .filter((row) => /^(follow|reverse|segment-pending|segment-head) /.test(row));
const caseNames = ['follow', 'reverse', 'segment-pending', 'segment-head'];
if (rows.length !== caseNames.length) {
  throw new Error(`Turbo Pascal Centipede tick output should contain ${caseNames.length} cases; got ${rows.length}`);
}

const pascalValues = [];
for (let i = 0; i < rows.length; i++) {
  const match = rows[i].match(/^(follow|reverse|segment-pending|segment-head) seed (-?\d+) draws (.*?) count (-?\d+) (.*?) row (.*?) seed-after (-?\d+)$/);
  if (!match) throw new Error(`Malformed Turbo Pascal Centipede tick case ${i + 1}: ${rows[i]}`);
  const expectedSeed = [0, 256, 0, 0][i];
  const expectedSeedAfter = [1, 1260005663, 0, 0][i];
  if (match[1] !== caseNames[i]
      || Number(match[2]) !== expectedSeed
      || Number(match[7]) !== expectedSeedAfter) {
    throw new Error(`Unexpected Turbo Pascal Centipede fixture state in case ${i + 1}: ${rows[i]}`);
  }

  const drawText = match[3].trim();
  const draws = drawText ? drawText.split(/\s+/).map(Number) : [];
  const statCount = Number(match[4]);
  const statValues = Array.from({ length: 3 }, () => Array(8).fill(-1));
  const statPattern = /s([123]) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+)/g;
  let statMatch;
  let parsedStats = 0;
  let remainder = match[5];
  while ((statMatch = statPattern.exec(match[5])) !== null) {
    const statIndex = Number(statMatch[1]) - 1;
    statValues[statIndex] = statMatch.slice(2).map(Number);
    parsedStats++;
  }
  remainder = remainder.replace(statPattern, '').trim();
  if (remainder || parsedStats !== statCount) {
    throw new Error(`Malformed Centipede stat snapshot in case ${i + 1}: ${match[5]}`);
  }

  const tiles = match[6].trim().split(/\s+/).map(Number);
  if (tiles.length !== 8) throw new Error(`Expected four Centipede row tiles in case ${i + 1}`);
  const randomMaxima = i === 0 ? [10, -1, -1]
    : i === 1 ? [10, 2, 2] : [-1, -1, -1];
  pascalValues.push(
    i + 1,
    draws.length,
    ...randomMaxima,
    statCount,
    ...statValues.flat(),
    ...tiles,
  );
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_centipede_tick_capture_checks();
  if (status !== 0) throw new Error(`Production Centipede tick capture returned ${status}`);

  const pointer = instance.exports.centipede_tick_output_ptr();
  const productionValues = new Int16Array(instance.exports.memory.buffer, pointer, 4 * 38);
  for (let i = 0; i < pascalValues.length; i++) {
    if (productionValues[i] !== pascalValues[i]) {
      const caseIndex = Math.floor(i / 38);
      const field = i % 38;
      throw new Error(`Centipede tick case ${caseIndex + 1} field ${field + 1} differs: production ${productionValues[i]}, Turbo Pascal ${pascalValues[i]}`);
    }
  }

  console.log('Turbo Pascal versus production Centipede ticks: chain following, boxed reversal, segment pending state, head conversion, random-call order, stat links, and tile/colors match across four cases.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
