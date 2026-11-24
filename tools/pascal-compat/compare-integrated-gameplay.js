const fs = require('node:fs');

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
if (!pascalPath || !wasmPath) throw new Error('Usage: compare-integrated-gameplay.js <INTEGPR.OUT> <integrated_gameplay.wasm>');

const cellCount = 62 * 27;
const fieldCount = 51 + cellCount * 2;
const checkpointSpecs = [
  { kind: 'L', id: 0, name: 'loaded initial state' },
  { kind: 'S', id: 0, name: 'game-loop start' },
  ...Array.from({ length: 6 }, (_, index) => ({ kind: 'C', id: index + 1, name: `cycle ${index + 1}` })),
  { kind: 'F', id: 0, name: 'final state' },
];
const expectedLines = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (expectedLines.length !== checkpointSpecs.length) {
  throw new Error(`Pascal integrated replay has ${expectedLines.length} checkpoints; expected ${checkpointSpecs.length}`);
}

const expectedSnapshots = expectedLines.map((line, rowIndex) => {
  const fields = line.trim().split(/\s+/);
  const spec = checkpointSpecs[rowIndex];
  if (fields[0] !== spec.kind || Number(fields[1]) !== spec.id || Number(fields[2]) !== fieldCount) {
    throw new Error(`Malformed Pascal checkpoint ${rowIndex + 1}; expected ${spec.kind} ${spec.id} ${fieldCount}, got ${fields.slice(0, 3).join(' ')}`);
  }
  const values = fields.slice(3).map(Number);
  if (values.length !== fieldCount || values.some(value => !Number.isInteger(value))) {
    throw new Error(`Pascal checkpoint ${spec.name} has ${values.length} fields; expected ${fieldCount} integers`);
  }
  return values;
});

const scalarNames = [
  'ammo', 'gems', 'health', 'current board', 'torches', 'torch ticks',
  'energizer ticks', 'score', 'board seconds', 'board hundredths', 'save flag', 'stat count',
];
const statNames = [
  'x', 'y', 'step x', 'step y', 'cycle', 'p1', 'p2', 'p3',
  'follower', 'leader', 'under element', 'under color', 'data position', 'data length',
];
const schedulerNames = [
  'current tick', 'current stat tick', 'tick speed', 'tick duration', 'tick counter',
  'paused', 'exit requested', 'input delta x', 'input delta y', 'key pressed', 'key buffer length',
];
function describeField(index) {
  if (index < scalarNames.length) return scalarNames[index];
  if (index < 40) {
    const offset = index - 12;
    return `stat ${Math.floor(offset / 14)} ${statNames[offset % 14]}`;
  }
  if (index < 51) return schedulerNames[index - 40];
  const tileField = index - 51;
  const tileIndex = Math.floor(tileField / 2);
  const x = Math.floor(tileIndex / 27);
  const y = tileIndex % 27;
  return `tile (${x},${y}) ${tileField % 2 === 0 ? 'element' : 'color'}`;
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

  const snapshotCount = instance.exports.integrated_gameplay_snapshot_count();
  if (snapshotCount !== checkpointSpecs.length) {
    throw new Error(`Production integrated replay has ${snapshotCount} checkpoints; expected ${checkpointSpecs.length}`);
  }
  const headers = new Int16Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_checkpoint_headers_ptr(), snapshotCount * 2);
  const actual = new Int16Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_snapshots_ptr(), snapshotCount * fieldCount);
  let comparedFields = 0;
  for (let row = 0; row < checkpointSpecs.length; row++) {
    const spec = checkpointSpecs[row];
    const expectedKind = spec.kind.charCodeAt(0);
    const actualKind = headers[row * 2];
    const actualId = headers[row * 2 + 1];
    if (actualKind !== expectedKind || actualId !== spec.id) {
      throw new Error(`First divergence at ${spec.name}: checkpoint identity expected=${spec.kind}/${spec.id}, actual=${String.fromCharCode(actualKind)}/${actualId}`);
    }
    const actualStart = row * fieldCount;
    for (let field = 0; field < fieldCount; field++) {
      const expected = expectedSnapshots[row][field];
      const observed = actual[actualStart + field];
      if (observed !== expected) {
        const from = Math.max(0, field - 2);
        const to = Math.min(fieldCount - 1, field + 2);
        const context = [];
        for (let at = from; at <= to; at++) {
          context.push(`${describeField(at)}:${expectedSnapshots[row][at]}->${actual[actualStart + at]}`);
        }
        throw new Error(`First divergence at ${spec.name}, field ${field} (${describeField(field)}): Pascal=${expected}, production=${observed}; context=[${context.join(', ')}]`);
      }
      comparedFields++;
    }
  }

  const final = new Int16Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_output_ptr(), fieldCount);
  if (final[1] !== 1 || final[11] !== 1 || final[45] !== 0 || final[46] !== 1) {
    throw new Error(`Integrated gameplay invariant failed: gems=${final[1]}, stats=${final[11]}, paused=${final[45]}, exit=${final[46]}`);
  }
  console.log(`Integrated gameplay replay: ${checkpointSpecs.length} checkpoints × ${fieldCount} fields matched (${comparedFields} comparisons); first-divergence reports identify checkpoint, field, tile, and surrounding values`);
})().catch(error => { console.error(error); process.exitCode = 1; });
