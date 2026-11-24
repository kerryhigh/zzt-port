const fs = require('node:fs');

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
if (!pascalPath || !wasmPath) throw new Error('Usage: compare-integrated-gameplay.js <INTEGPR.OUT> <oop_movement.wasm>');

const expected = fs.readFileSync(pascalPath, 'ascii').trim().split(/\s+/).map(Number);
const cellCount = 62 * 27;
const fieldCount = 51 + cellCount * 2;
if (expected.length !== fieldCount || expected.some(value => !Number.isInteger(value))) {
  throw new Error(`Pascal integrated replay has ${expected.length} values; expected ${fieldCount}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  let instance;
  let heapTop = 8 * 1024 * 1024;
  const allocations = new Map();
  const malloc = size => {
    const length = Math.max(1, Number(size) >>> 0);
    const pointer = heapTop;
    heapTop = (heapTop + length + 7) & ~7;
    if (heapTop > 15 * 1024 * 1024) return 0;
    allocations.set(pointer, length);
    return pointer;
  };
  const env = new Proxy({}, {
    get: (_target, name) => {
      if (name === 'malloc') return malloc;
      if (name === 'free') return pointer => { allocations.delete(Number(pointer)); };
      if (name === 'realloc') return (pointer, size) => {
        pointer = Number(pointer);
        const length = Math.max(1, Number(size) >>> 0);
        if (!pointer) return malloc(length);
        const oldLength = allocations.get(pointer) || 0;
        if (length <= oldLength) {
          allocations.set(pointer, length);
          return pointer;
        }
        const nextPointer = malloc(length);
        if (!nextPointer) return 0;
        if (instance) new Uint8Array(instance.exports.memory.buffer).copyWithin(nextPointer, pointer, pointer + oldLength);
        return nextPointer;
      };
      return () => 0;
    }
  });
  instance = await WebAssembly.instantiate(module, { env });
  const result = instance.exports.run_integrated_gameplay_checks();
  if (result !== 0) throw new Error(`Production integrated replay returned ${result}`);
  const actual = new Int16Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_output_ptr(), fieldCount);
  const fieldNames = [
    'ammo', 'gems', 'health', 'current board', 'torches', 'torch ticks',
    'energizer ticks', 'score', 'board seconds', 'board hundredths', 'save flag',
    'stat count',
  ];
  for (let i = 0; i < fieldCount; i++) {
    if (actual[i] !== expected[i]) {
      if (i < fieldNames.length) {
        throw new Error(`Integrated Pascal/production mismatch in ${fieldNames[i]}: ${expected[i]} != ${actual[i]}`);
      }
      if (i < 51) {
        throw new Error(`Integrated Pascal/production mismatch in state field ${i}: ${expected[i]} != ${actual[i]}`);
      }
      const tileField = i - 51;
      const tileIndex = Math.floor(tileField / 2);
      const x = Math.floor(tileIndex / 27);
      const y = tileIndex % 27;
      const component = tileField % 2 === 0 ? 'element' : 'color';
      throw new Error(`Integrated Pascal/production mismatch at board (${x},${y}) ${component}: ${expected[i]} != ${actual[i]}`);
    }
  }
  console.log(`Integrated gameplay differential: all ${fieldCount} fields match compiled Turbo Pascal 5.5, including 1,674 board cells, both stats, counters, scheduler state, and queued input`);
})().catch(error => { console.error(error); process.exitCode = 1; });
