const fs = require('node:fs');

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
const savedWorldPlayWorldPath = process.argv[4] === '--saved-world-play' ? process.argv[5] || null : null;
const restoreFailureSavePath = process.argv[4] === '--restore-failure' ? process.argv[5] || null : null;
const restoreRetrySavePath = process.argv[4] === '--restore-retry' ? process.argv[5] || null : null;
const restoreSavePath = savedWorldPlayWorldPath || restoreFailureSavePath || restoreRetrySavePath
  ? null
  : process.argv[4] || null;
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: compare-title-lifecycle.js <INTEGPR.OUT|TITLEFAIL.OUT> <integrated_gameplay.wasm> [--saved-world-play world.ZZT|restore.SAV|--restore-failure malformed.SAV|--restore-retry restore.SAV]');
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

function parsePascalSnapshots(path, includeCancel = false) {
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
  const titleCancel = rows.find(row => row.kind === 'X' && row.id === 0);
  if (!playerStart || !playerCycle || !titleReturn) {
    throw new Error(`Title lifecycle capture missing checkpoints: P=${Boolean(playerStart)}, player C1=${Boolean(playerCycle)}, T=${Boolean(titleReturn)}`);
  }
  if (includeCancel && !titleCancel) throw new Error('Title restore retry capture is missing the X/0 post-cancel checkpoint.');
  return includeCancel
    ? [titleCancel, playerStart, playerCycle, titleReturn]
    : [playerStart, playerCycle, titleReturn];
}

function parsePascalRestoreFailure(path) {
  const text = fs.readFileSync(path, 'ascii').trim();
  const values = text.split(/\s+/).map(Number);
  const expectedCount = 38 + (62 * 27 * 2);
  if (values.length !== expectedCount || values.some(value => !Number.isInteger(value))) {
    throw new Error(`Malformed Pascal title-restore failure capture: ${values.length} fields, expected ${expectedCount}`);
  }
  return values;
}

function describeRestoreFailureField(index) {
  const fixed = [
    'GameWorldLoad accepted', 'World.BoardCount', 'World.BoardLen[0]', 'ReturnBoardId',
    'World.Info.CurrentBoard', 'ammo', 'gems', 'health', 'torches', 'torch ticks',
    'energizer ticks', 'score', 'board time seconds', 'board time hundredths', 'save flag',
    'Board.StatCount', 'InputKeyPressed',
  ];
  if (index < fixed.length) return fixed[index];
  const statFields = [
    'x', 'y', 'step x', 'step y', 'cycle', 'p1', 'p2', 'p3',
    'follower', 'leader', 'under element', 'under color', 'data position', 'data length',
  ];
  if (index < 31) return `Board.Stats[0].${statFields[index - fixed.length]}`;
  if (index < 38) return `World.Info.Keys[${index - 30}]`;
  const tileIndex = Math.floor((index - 38) / 2);
  return `tile (${Math.floor(tileIndex / 27)},${tileIndex % 27}) ${(index - 38) % 2 ? 'color' : 'element'}`;
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

function readProductionSnapshots(instance, expectedCount) {
  const exports = instance.exports;
  const count = exports.integrated_gameplay_snapshot_count();
  if (count !== expectedCount) throw new Error(`Production lifecycle emitted ${count} snapshots; expected ${expectedCount}`);
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
  const expected = restoreFailureSavePath
    ? parsePascalRestoreFailure(pascalPath)
    : parsePascalSnapshots(pascalPath, Boolean(restoreRetrySavePath));
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
  let result;
  if (restoreFailureSavePath) {
    const save = fs.readFileSync(restoreFailureSavePath);
    if (save.length !== 512) throw new Error(`Malformed restore fixture is ${save.length} bytes; expected the direct-Pascal 512-byte header.`);
    const capacity = instance.exports.integrated_demo_world_bytes_capacity();
    if (save.length > capacity) throw new Error(`Restore fixture exceeds production harness capacity: ${save.length}/${capacity}`);
    const pointer = instance.exports.integrated_demo_world_bytes_ptr();
    new Uint8Array(instance.exports.memory.buffer, pointer, save.length).set(save);
    const result = instance.exports.run_integrated_title_restore_failure_checks(save.length);
    if (result !== 0) throw new Error(`Production malformed title restore harness returned ${result}`);
    const error = instance.exports.integrated_title_restore_failure_error();
    if (error !== -3) throw new Error(`Production malformed title restore returned ${error}; expected ZOO_ERROR_WRONGVER (-3).`);
    const count = instance.exports.integrated_title_restore_failure_field_count();
    if (count !== expected.length) throw new Error(`Production malformed title restore emitted ${count} fields; Pascal emitted ${expected.length}.`);
    const actual = Array.from(new Int32Array(
      instance.exports.memory.buffer,
      instance.exports.integrated_title_restore_failure_fields_ptr(),
      count,
    ));
    let comparedFields = 0;
    for (let field = 0; field < expected.length; field++) {
      // Pascal's World.BoardLen is the byte length of its raw in-memory board
      // record. LibZoo's corresponding private buffer also carries explicit
      // Pascal short-string preservation metadata, so that internal length is
      // intentionally not a cross-implementation state field. The actual DOS
      // file bytes remain covered by the serializer differential suite.
      if (field === 2) {
        if (actual[field] <= 0) throw new Error(`Production retained invalid internal board length ${actual[field]} after malformed restore.`);
        continue;
      }
      if (expected[field] !== actual[field]) {
        const from = Math.max(0, field - 2);
        const to = Math.min(expected.length - 1, field + 2);
        const context = [];
        for (let near = from; near <= to; near++) {
          context.push({ field: describeRestoreFailureField(near), pascal: expected[near], production: actual[near] });
        }
        throw new Error(`First malformed title-restore divergence at field ${field} (${describeRestoreFailureField(field)}): Pascal=${expected[field]}, production=${actual[field]}; nearby=${JSON.stringify(context)}`);
      }
      comparedFields++;
    }
    if (expected[0] !== 0) throw new Error(`Pascal GameWorldLoad accepted the malformed .SAV (result=${expected[0]}).`);
    console.log(`Title R malformed-version refusal matched Pascal across ${comparedFields}/${count} comparable world/board/stat/tile fields; private board-buffer length was excluded from equality; LibZoo returned ZOO_ERROR_WRONGVER (-3).`);
    return;
  }
  if (savedWorldPlayWorldPath) {
    const world = fs.readFileSync(savedWorldPlayWorldPath);
    const capacity = instance.exports.integrated_demo_world_bytes_capacity();
    if (world.length > capacity) throw new Error(`World fixture exceeds production harness capacity: ${world.length}/${capacity}`);
    const pointer = instance.exports.integrated_demo_world_bytes_ptr();
    new Uint8Array(instance.exports.memory.buffer, pointer, world.length).set(world);
    result = instance.exports.run_integrated_title_saved_world_play_checks(world.length);
  } else if (restoreRetrySavePath) {
    const save = fs.readFileSync(restoreRetrySavePath);
    const capacity = instance.exports.integrated_demo_world_bytes_capacity();
    if (save.length > capacity) throw new Error(`Restore fixture exceeds production harness capacity: ${save.length}/${capacity}`);
    const pointer = instance.exports.integrated_demo_world_bytes_ptr();
    new Uint8Array(instance.exports.memory.buffer, pointer, save.length).set(save);
    result = instance.exports.run_integrated_title_restore_retry_checks(save.length);
  } else if (restoreSavePath) {
    const save = fs.readFileSync(restoreSavePath);
    const capacity = instance.exports.integrated_demo_world_bytes_capacity();
    if (save.length > capacity) throw new Error(`Restore fixture exceeds production harness capacity: ${save.length}/${capacity}`);
    const pointer = instance.exports.integrated_demo_world_bytes_ptr();
    new Uint8Array(instance.exports.memory.buffer, pointer, save.length).set(save);
    result = instance.exports.run_integrated_title_restore_checks(save.length);
  } else {
    result = instance.exports.run_integrated_title_lifecycle_checks();
  }
  if (result !== 0) throw new Error(`Production title lifecycle returned ${result}`);
  const actual = readProductionSnapshots(instance, expected.length);
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
  const scenario = restoreRetrySavePath
    ? 'R-to-cancel selection, retry R restore from the same Pascal .SAV, one real paused cycle, and return-to-title'
    : restoreSavePath
    ? 'R-to-restore from the same Pascal .SAV, one real paused cycle, and return-to-title'
    : savedWorldPlayWorldPath
    ? 'title-P reload of the backing Pascal .ZZT, one real paused cycle, and return-to-title'
    : 'P-to-play, one real paused cycle, and return-to-title';
  console.log(`Title lifecycle replay: ${scenario} matched at ${expected.length} checkpoints (${comparedFields} state fields); first-divergence context includes neighboring fields.`);
})().catch(error => { console.error(error); process.exitCode = 1; });
