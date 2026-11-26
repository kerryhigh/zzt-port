const fs = require('node:fs');

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: compare-title-lifecycle.js <INTEGPR.OUT> <integrated_gameplay.wasm>');
}

const scalarNames = [
  'ammo', 'gems', 'health', 'current board', 'torches', 'torch ticks',
  'energizer ticks', 'score', 'board seconds', 'board hundredths', 'save flag', 'stat count',
  'game state element (player/monitor role)',
];
const statNames = [
  'x', 'y', 'step x', 'step y', 'cycle', 'p1', 'p2', 'p3',
  'follower', 'leader', 'under element', 'under color', 'data position', 'data length',
];
const schedulerNames = [
  'current tick', 'current stat tick', 'tick speed', 'tick duration', 'tick counter',
  'paused', 'exit requested', 'input delta x', 'input delta y', 'key pressed', 'key buffer length',
];

function parsePascalSnapshots(path) {
  const rows = fs.readFileSync(path, 'ascii').trim().split(/\r?\n/).map((line, lineIndex) => {
    const fields = line.trim().split(/\s+/);
    if (fields.length < 3) throw new Error(`Malformed Pascal snapshot at line ${lineIndex + 1}`);
    const kind = fields[0];
    const id = Number(fields[1]);
    const fieldCount = Number(fields[2]);
    const values = fields.slice(3).map(Number);
    if (!Number.isInteger(id) || !Number.isInteger(fieldCount) || values.length !== fieldCount
        || values.some(value => !Number.isInteger(value))) {
      throw new Error(`Malformed ${kind}/${id} Pascal snapshot at line ${lineIndex + 1}`);
    }
    return { kind, id, values, lineIndex: lineIndex + 1 };
  });
  const playerStart = rows.find(row => row.kind === 'P' && row.id === 0);
  const playerCycle = rows.find(row => row.kind === 'C' && row.id === 1 && row.values[12] === 1);
  const titleReturn = rows.find(row => row.kind === 'T' && row.id === 0);
  if (!playerStart || !playerCycle || !titleReturn) {
    throw new Error(`Title lifecycle capture missing checkpoints: P=${Boolean(playerStart)}, player C1=${Boolean(playerCycle)}, T=${Boolean(titleReturn)}`);
  }
  return [playerStart, playerCycle, titleReturn];
}

function describeField(index, values) {
  if (index < scalarNames.length) return scalarNames[index];
  const statCount = values[11];
  const statSlots = statCount + 1;
  const statEnd = scalarNames.length + statSlots * 14;
  if (index < statEnd) {
    const offset = index - scalarNames.length;
    return `stat ${Math.floor(offset / 14)} ${statNames[offset % 14]}`;
  }
  const schedulerEnd = statEnd + schedulerNames.length;
  if (index < schedulerEnd) return schedulerNames[index - statEnd];
  const tileStart = schedulerEnd;
  const tileEnd = tileStart + (62 * 27 * 2);
  if (index < tileEnd) {
    const offset = index - tileStart;
    const tile = Math.floor(offset / 2);
    return `tile (${Math.floor(tile / 27)},${tile % 27}) ${offset % 2 ? 'color' : 'element'}`;
  }
  if (index === tileEnd) return 'RNG seed low word';
  if (index === tileEnd + 1) return 'RNG seed high word';
  return `snapshot field ${index}`;
}

function readProductionSnapshots(instance) {
  const exports = instance.exports;
  const count = exports.integrated_gameplay_snapshot_count();
  if (count !== 3) throw new Error(`Production lifecycle emitted ${count} snapshots; expected 3`);
  const memory = exports.memory.buffer;
  const headers = new Int16Array(memory, exports.integrated_gameplay_checkpoint_headers_ptr(), count * 2);
  const lengths = new Int32Array(memory, exports.integrated_gameplay_snapshot_lengths_ptr(), count);
  const stride = exports.integrated_gameplay_snapshot_stride();
  const flat = new Int32Array(memory, exports.integrated_gameplay_snapshots_ptr(), count * stride);
  return Array.from({ length: count }, (_, index) => ({
    kind: String.fromCharCode(headers[index * 2]),
    id: headers[index * 2 + 1],
    values: Array.from(flat.slice(index * stride, index * stride + lengths[index])),
  }));
}

(async () => {
  const expected = parsePascalSnapshots(pascalPath);
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
        const next = malloc(length);
        if (next && instance) new Uint8Array(instance.exports.memory.buffer).copyWithin(next, pointer, pointer + oldLength);
        return next;
      };
      return () => 0;
    },
  });
  instance = await WebAssembly.instantiate(module, { env });
  const result = instance.exports.run_integrated_title_lifecycle_checks();
  if (result !== 0) throw new Error(`Production title lifecycle returned ${result}`);
  const actual = readProductionSnapshots(instance);
  let comparedFields = 0;
  for (let checkpoint = 0; checkpoint < expected.length; checkpoint++) {
    const want = expected[checkpoint];
    const got = actual[checkpoint];
    if (got.kind !== want.kind || got.id !== want.id) {
      throw new Error(`Checkpoint ${checkpoint + 1} identity differs: Pascal=${want.kind}/${want.id}, production=${got.kind}/${got.id}`);
    }
    if (got.values.length !== want.values.length) {
      throw new Error(`Checkpoint ${want.kind}/${want.id} field count differs: Pascal=${want.values.length}, production=${got.values.length}`);
    }
    for (let field = 0; field < want.values.length; field++) {
      if (want.values[field] !== got.values[field]) {
        const from = Math.max(0, field - 2);
        const to = Math.min(want.values.length - 1, field + 2);
        const context = [];
        for (let near = from; near <= to; near++) {
          context.push({ field: describeField(near, want.values), pascal: want.values[near], production: got.values[near] });
        }
        throw new Error(`First title lifecycle divergence at Pascal line ${want.lineIndex}, checkpoint ${want.kind}/${want.id}, field ${field} (${describeField(field, want.values)}): Pascal=${want.values[field]}, production=${got.values[field]}; nearby=${JSON.stringify(context)}`);
      }
      comparedFields++;
    }
  }
  console.log(`Title lifecycle replay: P-to-play, one real paused cycle, and return-to-title matched at ${expected.length} checkpoints (${comparedFields} state fields); first-divergence context includes neighboring fields.`);
})().catch(error => { console.error(error); process.exitCode = 1; });
