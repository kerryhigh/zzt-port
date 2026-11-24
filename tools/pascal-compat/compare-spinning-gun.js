const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-spinning-gun.js SPINGUN.OUT spinning_gun.wasm');
}

function parsePascalTrace(line) {
  const common = /^(.*?) stats (-?\d+) player (-?\d+) (-?\d+) gun (-?\d+) (-?\d+) rng (-?\d+) (.+)$/.exec(line);
  if (!common) throw new Error(`Unexpected Pascal Spinning Gun trace: ${line}`);

  const trace = [
    Number(common[2]), Number(common[3]), Number(common[4]),
    Number(common[5]), Number(common[6]), Number(common[7]),
    0, 0, 0, 0, 0, 0,
  ];
  if (common[8] === 'no-projectile') return trace;

  const projectile = /^projectile (-?\d+) (-?\d+) (-?\d+) step (-?\d+) (-?\d+) p1 (-?\d+) p2 (-?\d+) under (-?\d+) (-?\d+)$/.exec(common[8]);
  if (!projectile) throw new Error(`Unexpected Pascal Spinning Gun projectile trace: ${line}`);
  trace[6] = Number(projectile[1]);
  trace[7] = Number(projectile[2]);
  trace[8] = Number(projectile[3]);
  trace[9] = Number(projectile[4]);
  trace[10] = Number(projectile[5]);
  trace[11] = Number(projectile[6]);
  return trace;
}

async function main() {
  const lines = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const { exports } = instance;
  const result = exports.run_spinning_gun_checks();
  if (result !== 0) throw new Error(`Production Spinning Gun checks returned ${result}`);

  const count = exports.get_spinning_gun_trace_count();
  if (count !== lines.length) {
    throw new Error(`Production produced ${count} Spinning Gun cases; Pascal produced ${lines.length}.`);
  }
  const pointer = exports.get_spinning_gun_trace() >>> 0;
  const view = new Int32Array(exports.memory.buffer, pointer, count * 12);
  for (let index = 0; index < count; index++) {
    const actual = Array.from(view.subarray(index * 12, (index + 1) * 12));
    const expected = parsePascalTrace(lines[index]);
    if (actual.some((value, field) => value !== expected[field])) {
      throw new Error(`Spinning Gun case ${index + 1} (${lines[index].split(' ')[0]}) differs: `
        + `production [${actual.join(', ')}], Pascal [${expected.join(', ')}]`);
    }
  }
  console.log(`Compiled Turbo Pascal versus production Spinning Gun tick: ${count} complete trace rows match player/gun position, stat count, shot request, and signed RNG state.`);
}

main().catch((error) => {
  process.stderr.write(`${error.stack || error}\n`);
  process.exitCode = 1;
});
