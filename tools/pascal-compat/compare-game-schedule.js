const fs = require('node:fs');

const [pascalOutputPath, productionWasmPath] = process.argv.slice(2);
if (!pascalOutputPath || !productionWasmPath) {
  throw new Error('Usage: compare-game-schedule.js GAMESCHD.OUT tick_exit.wasm');
}

const lines = fs.readFileSync(pascalOutputPath, 'ascii').trim().split(/\r?\n/);
if (lines.length !== 9) throw new Error(`Expected nine Pascal schedule rows; found ${lines.length}`);
const expectedCases = [
  [0, 0, 0, 0], [0, 5980, 0, 0],
  [1, 0, 0, 0], [1, 5980, 0, 0],
  [6, 0, 0, 0], [6, 5980, 0, 0],
  [9, 0, 0, 0], [9, 5980, 0, 0], [6, 0, 2, 4],
];
const expectedRows = lines.map((line, index) => {
  const fields = line.trim().split(/\s+/);
  if (fields.length !== 22 || fields[0] !== 'speed' || fields[2] !== 'start'
      || fields[4] !== 'stall' || fields[6] !== 'extra' || fields[8] !== 'cycles'
      || fields[10] !== 'delta' || fields[12] !== 'stat' || fields[14] !== 'counter'
      || fields[16] !== 'times') {
    throw new Error(`Malformed Pascal GamePlayLoop schedule row ${index + 1}: ${line}`);
  }
  const row = [Number(fields[1]), Number(fields[3]), Number(fields[5]), Number(fields[7]),
    Number(fields[9]), Number(fields[11]), Number(fields[13]), Number(fields[15]),
    ...fields.slice(17).map(Number)];
  if (row.length !== 13 || row.slice(0, 4).some((value, field) => value !== expectedCases[index][field])
      || row[4] !== 5 || row[5] !== 5 || row[6] !== 0 || row[7] !== row[12]) {
    throw new Error(`Unexpected Pascal schedule state in row ${index + 1}: ${line}`);
  }
  return row;
});

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(productionWasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const result = instance.exports.run_game_schedule_checks();
  if (result !== 0) throw new Error(`Production schedule check returned ${result}`);
  const pointer = instance.exports.game_schedule_output_ptr();
  const actualRows = new Int16Array(instance.exports.memory.buffer, pointer, 117);
  for (let row = 0; row < expectedRows.length; row++) {
    for (let field = 0; field < 13; field++) {
      const actual = actualRows[row * 13 + field];
      const expected = expectedRows[row][field];
      if (actual !== expected) {
        throw new Error(`GamePlayLoop schedule row ${row + 1}, field ${field + 1} differs: production=${actual}, Pascal=${expected}`);
      }
    }
  }
  console.log('Turbo Pascal GamePlayLoop versus production schedule: speeds 0, 1, 6, and 9 at phases 0 and 5980, plus a four-PIT stall, match for five cycles.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
