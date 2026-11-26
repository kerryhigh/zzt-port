const fs = require('node:fs');

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
const soundPath = process.argv[4];
const cellPath = process.argv[5];
const savePath = process.argv[6];
const loadResultPath = process.argv[7];
const roundtripSavePath = process.argv[8];
const scenario = process.argv[9] || 'synthetic';
const demoWorldPath = process.argv[10];
if (!pascalPath || !wasmPath || !soundPath || !cellPath || !savePath || !loadResultPath || !roundtripSavePath) throw new Error('Usage: compare-integrated-gameplay.js <INTEGPR.OUT> <integrated_gameplay.wasm> <INTEGSND.OUT> <INTEGCEL.OUT> <INTEGSAV.SAV> <INTEGLOD.OUT> <INTEG2SV.SAV> [synthetic|demo-traversal|walls-pushers-duplicators|combat-keys-pickups|object-stat-order|board-neighbors-passages|dark-room-torch-energizer-timers-death [DEMO.ZZT]]');
if (!['synthetic', 'demo-traversal', 'walls-pushers-duplicators', 'combat-keys-pickups', 'object-stat-order', 'board-neighbors-passages', 'dark-room-torch-energizer-timers-death'].includes(scenario)) throw new Error(`Unknown integrated replay scenario: ${scenario}`);
if (scenario === 'demo-traversal' && !demoWorldPath) throw new Error('DEMO traversal comparison requires the source DEMO.ZZT file');

const cellCount = 62 * 27;
const tileFieldCount = cellCount * 2;
const statFieldCount = 14;
const schedulerFieldCount = 11;
const expectedLines = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const checkpointSpecs = [];
const replayTransitionCount = scenario === 'board-neighbors-passages' ? 4 : 3;
const postCycleStateIds = scenario === 'board-neighbors-passages' ? [2, 3] : [];
const expectedCycleCount = scenario === 'dark-room-torch-energizer-timers-death' ? 120 : 6;
let postCycleStateIndex = 0;
let nextTransitionId = 1;
let nextCycleId = 1;
const checkpointHeaders = expectedLines.map((line, rowIndex) => {
  const fields = line.trim().split(/\s+/);
  if (fields.length < 3) throw new Error(`Malformed Pascal checkpoint header at row ${rowIndex + 1}`);
  return { kind: fields[0], id: Number(fields[1]), fieldCount: Number(fields[2]), fields };
});
if (checkpointHeaders.length < expectedCycleCount + 6
    || checkpointHeaders[0].kind !== 'L' || checkpointHeaders[0].id !== 0
    || checkpointHeaders[1].kind !== 'S' || checkpointHeaders[1].id !== 0) {
  throw new Error('Integrated replay must start with L0 loaded state followed by S0 game-loop start');
}
checkpointSpecs.push(
  { kind: 'L', id: 0, name: 'loaded initial state' },
  { kind: 'S', id: 0, name: 'game-loop start' },
);
for (let row = 2; row < checkpointHeaders.length; row++) {
  const header = checkpointHeaders[row];
  if (header.kind === 'T' && header.id === nextTransitionId && nextTransitionId <= replayTransitionCount) {
    checkpointSpecs.push({ kind: 'T', id: header.id, name: `state transition ${header.id}` });
    nextTransitionId++;
  } else if (header.kind === 'C' && header.id === nextCycleId && nextCycleId <= expectedCycleCount) {
    checkpointSpecs.push({ kind: 'C', id: header.id, name: `cycle ${header.id}` });
    nextCycleId++;
  } else if (nextCycleId === 7 && postCycleStateIndex < postCycleStateIds.length
      && header.kind === 'T' && header.id === postCycleStateIds[postCycleStateIndex]) {
    const snapshotName = postCycleStateIndex === 0 ? 'post-save state' : 'post-load state';
    checkpointSpecs.push({ kind: 'T', id: header.id, name: snapshotName });
    postCycleStateIndex++;
  } else if (header.kind === 'F' && header.id === 0 && row === checkpointHeaders.length - 1) {
    checkpointSpecs.push({ kind: 'F', id: 0, name: 'final state' });
  } else {
    throw new Error(`Invalid checkpoint order or identity at row ${row + 1}: ${header.kind}/${header.id}`);
  }
}
if (checkpointSpecs.at(-1)?.kind !== 'F' || nextCycleId !== expectedCycleCount + 1
    || nextTransitionId !== replayTransitionCount + 1
    || postCycleStateIndex !== postCycleStateIds.length) {
  throw new Error(`Integrated replay requires ${replayTransitionCount} ordered gameplay transitions, ${expectedCycleCount} ordered cycles, ${postCycleStateIds.length} post-cycle save/load snapshots, and a final state; transitions=${nextTransitionId - 1}, cycles=${nextCycleId - 1}, postCycleSnapshots=${postCycleStateIndex}`);
}
const expectedSnapshots = expectedLines.map((line, rowIndex) => {
  const fields = line.trim().split(/\s+/);
  const spec = checkpointSpecs[rowIndex];
  const fieldCount = Number(fields[2]);
  if (fields[0] !== spec.kind || Number(fields[1]) !== spec.id || !Number.isInteger(fieldCount) || fieldCount < 0) {
    throw new Error(`Malformed Pascal checkpoint ${rowIndex + 1}; expected ${spec.kind} ${spec.id} and a nonnegative field count, got ${fields.slice(0, 3).join(' ')}`);
  }
  const values = fields.slice(3).map(Number);
  if (values.length !== fieldCount || values.some(value => !Number.isInteger(value))) {
    throw new Error(`Pascal checkpoint ${spec.name} has ${values.length} fields; expected ${fieldCount} integers`);
  }
  return { fieldCount, values };
});

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
const soundNames = [
  'priority', 'buffer position', 'buffer length', 'playing',
  'enabled', 'queue blocked', 'duration counter', 'duration multiplier',
];
function describeField(index, statCount, values) {
  if (index < scalarNames.length) return scalarNames[index];
  const statSlots = statCount + 1;
  const statFieldEnd = scalarNames.length + statSlots * statFieldCount;
  if (index < statFieldEnd) {
    const offset = index - scalarNames.length;
    return `stat ${Math.floor(offset / statFieldCount)} ${statNames[offset % statFieldCount]}`;
  }
  const schedulerStart = statFieldEnd;
  if (index < schedulerStart + schedulerFieldCount) return schedulerNames[index - schedulerStart];
  const tileStart = schedulerStart + schedulerFieldCount;
  const tileFieldEnd = tileStart + tileFieldCount;
  if (index < tileFieldEnd) {
    const tileField = index - tileStart;
    const tileIndex = Math.floor(tileField / 2);
    const x = Math.floor(tileIndex / 27);
    const y = tileIndex % 27;
    return `tile (${x},${y}) ${tileField % 2 === 0 ? 'element' : 'color'}`;
  }
  if (index === tileFieldEnd) return 'RNG seed low word';
  if (index === tileFieldEnd + 1) return 'RNG seed high word';
  const aliasGraphSize = statSlots;
  const aliasGraphFields = aliasGraphSize * aliasGraphSize;
  const aliasGraphStart = tileFieldEnd + 2;
  if (index < aliasGraphStart + aliasGraphFields) {
    const aliasIndex = index - tileFieldEnd - 2;
    return `script pointer alias stat ${Math.floor(aliasIndex / aliasGraphSize)}=${aliasIndex % aliasGraphSize}`;
  }
  let scriptByteIndex = index - aliasGraphStart - aliasGraphFields;
  let scriptBytes = 0;
  for (let statId = 0; statId <= statCount; statId++) {
    const dataLength = values[scalarNames.length + statId * statFieldCount + 13];
    let isFirstOwner = dataLength > 0;
    for (let earlierId = 0; earlierId < statId && isFirstOwner; earlierId++) {
      const earlierDataLength = values[scalarNames.length + earlierId * statFieldCount + 13];
      const aliasIndex = aliasGraphStart + statId * statSlots + earlierId;
      if (earlierDataLength > 0 && values[aliasIndex] !== 0) isFirstOwner = false;
    }
    const capturedLength = isFirstOwner ? dataLength : 0;
    if (scriptByteIndex < capturedLength) return `stat ${statId} script byte ${scriptByteIndex}`;
    scriptByteIndex -= capturedLength;
    scriptBytes += capturedLength;
  }
  const soundStart = aliasGraphStart + aliasGraphFields + scriptBytes;
  const soundField = index - soundStart;
  if (soundField >= 0 && soundField < soundNames.length) return `sound ${soundNames[soundField]}`;
  const soundBufferLength = values[soundStart + 2] || 0;
  if (soundField < soundNames.length + soundBufferLength) return `sound queue byte ${soundField - soundNames.length}`;
  if (soundField === soundNames.length + soundBufferLength) return 'captured playfield cell-write count';
  if (soundField === soundNames.length + soundBufferLength + 1) return 'playfield cell-write overflow';
  return `script byte ${scriptByteIndex} (unclassified)`;
}

function parseSoundEvents(path) {
  const raw = fs.readFileSync(path, 'ascii').trim();
  if (!raw) throw new Error('Pascal integrated sound-event output is empty');
  const tokens = raw.split(/\s+/).map(Number);
  if (tokens.some(value => !Number.isInteger(value))) throw new Error('Pascal integrated sound-event output contains a non-integer token');
  const count = tokens[0];
  const overflow = tokens[1];
  if (!Number.isInteger(count) || count < 0 || ![0, 1].includes(overflow)) {
    throw new Error('Pascal integrated sound-event header is malformed');
  }
  let cursor = 2;
  const events = [];
  for (let eventIndex = 0; eventIndex < count; eventIndex++) {
    if (cursor + 2 > tokens.length) throw new Error(`Pascal sound event ${eventIndex} is truncated before its request header`);
    const priority = tokens[cursor++];
    const requestLength = tokens[cursor++];
    if (requestLength < 0 || requestLength > 255 || cursor + requestLength + 4 > tokens.length) {
      throw new Error(`Pascal sound event ${eventIndex} has an invalid request length ${requestLength}`);
    }
    const requestBytes = tokens.slice(cursor, cursor + requestLength);
    cursor += requestLength;
    const resultPriority = tokens[cursor++];
    const resultPosition = tokens[cursor++];
    const resultLength = tokens[cursor++];
    const resultPlaying = tokens[cursor++];
    if (resultLength < 0 || resultLength > 255 || cursor + resultLength > tokens.length) {
      throw new Error(`Pascal sound event ${eventIndex} has an invalid result length ${resultLength}`);
    }
    const resultBytes = tokens.slice(cursor, cursor + resultLength);
    cursor += resultLength;
    events.push({ priority, requestLength, requestBytes, resultPriority, resultPosition, resultLength, resultPlaying, resultBytes });
  }
  if (cursor !== tokens.length) throw new Error(`Pascal sound-event output has ${tokens.length - cursor} unclassified trailing token(s)`);
  return { count, overflow, events };
}

function parseCellWrites(path) {
  const raw = fs.readFileSync(path, 'ascii').trim();
  if (!raw) throw new Error('Pascal integrated cell-write output is empty');
  const tokens = raw.split(/\s+/).map(Number);
  if (tokens.some(value => !Number.isInteger(value))) throw new Error('Pascal integrated cell-write output contains a non-integer token');
  const count = tokens[0];
  const overflow = tokens[1];
  if (!Number.isInteger(count) || count < 0 || ![0, 1].includes(overflow)) {
    throw new Error('Pascal integrated cell-write header is malformed');
  }
  const headerFields = tokens.length === 3 + (count * 4) ? 3 : 2;
  const idempotentWriteCount = headerFields === 3 ? tokens[2] : 0;
  if (tokens.length !== headerFields + (count * 4)
      || !Number.isInteger(idempotentWriteCount) || idempotentWriteCount < 0) {
    throw new Error(`Pascal integrated cell-write output has ${tokens.length - headerFields} data fields; expected ${count * 4}`);
  }
  const events = [];
  for (let eventIndex = 0; eventIndex < count; eventIndex++) {
    const base = headerFields + (eventIndex * 4);
    const event = { x: tokens[base], y: tokens[base + 1], color: tokens[base + 2], character: tokens[base + 3] };
    if (event.x < 0 || event.x >= 60 || event.y < 0 || event.y >= 25
        || event.color < 0 || event.color > 255 || event.character < 0 || event.character > 255) {
      throw new Error(`Pascal cell-write event ${eventIndex} is outside the captured playfield/byte range: ${JSON.stringify(event)}`);
    }
    events.push(event);
  }
  return { count, overflow, idempotentWriteCount, events };
}

function integratedSaveNondeterministicRanges(bytes, loadedFromFile = false) {
  const ranges = [];
  const add = (start, length, label) => {
    if (start < 0 || length < 0 || start + length > bytes.length) {
      throw new Error(`Serialized save field ${label} is outside the ${bytes.length}-byte file`);
    }
    if (length > 0) ranges.push({ start, length, label });
  };
  const emptyStringTail = (lengthOffset, maxLength, label, classifyUndefinedTail) => {
    if (lengthOffset < 0 || lengthOffset >= bytes.length) throw new Error(`Missing serialized string length for ${label}`);
    const length = bytes[lengthOffset];
    if (length > maxLength) throw new Error(`Serialized string ${label} has invalid length ${length}`);
    if (classifyUndefinedTail && length === 0) add(lengthOffset + 1, maxLength, `${label} empty-string tail`);
  };

  if (bytes.length < 514 || bytes.readInt16LE(0) !== -1) {
    throw new Error('Integrated save is not a version-marked ZZT world file');
  }
  emptyStringTail(29, 20, 'world name', !loadedFromFile);
  for (let flag = 0; flag < 10; flag++) {
    emptyStringTail(50 + (flag * 21), 20, `world flag ${flag + 1}`, !loadedFromFile);
  }
  if (!loadedFromFile) add(265, 14, 'WorldInfo.unkPad (uninitialized Pascal record bytes)');

  const boardCount = bytes.readInt16LE(2);
  if (boardCount < 0 || boardCount > 100) throw new Error(`Invalid integrated save board count ${boardCount}`);
  let fileOffset = 512;
  for (let boardId = 0; boardId <= boardCount; boardId++) {
    if (fileOffset + 2 > bytes.length) throw new Error(`Missing serialized board ${boardId} length`);
    const boardLength = bytes.readUInt16LE(fileOffset);
    const boardStart = fileOffset + 2;
    const boardEnd = boardStart + boardLength;
    if (boardEnd > bytes.length || boardLength < 51 + 86 + 2) {
      throw new Error(`Serialized board ${boardId} length ${boardLength} is truncated or too short`);
    }
    emptyStringTail(boardStart, 50, `board ${boardId} name`, !loadedFromFile);

    let boardInfoStart = boardStart + 51;
    let cells = 0;
    while (cells < 60 * 25) {
      if (boardInfoStart + 3 > boardEnd) throw new Error(`Serialized board ${boardId} RLE map is truncated`);
      const runLength = bytes[boardInfoStart];
      if (runLength === 0 || cells + runLength > 60 * 25) {
        throw new Error(`Serialized board ${boardId} has invalid RLE run ${runLength} after ${cells} cells`);
      }
      cells += runLength;
      boardInfoStart += 3;
    }
    emptyStringTail(boardInfoStart + 7, 58, `board ${boardId} message`, !loadedFromFile);
    if (!loadedFromFile) {
      add(boardInfoStart + 66, 2, `board ${boardId} uninitialized start-player coordinates`);
      add(boardInfoStart + 70, 16, `board ${boardId} TBoardInfo.unkPad`);
    }

    const statCount = bytes.readInt16LE(boardInfoStart + 86);
    if (statCount < 0 || statCount > 150) throw new Error(`Serialized board ${boardId} has invalid stat count ${statCount}`);
    let statOffset = boardInfoStart + 88;
    for (let statId = 0; statId <= statCount; statId++) {
      if (statOffset + 33 > boardEnd) throw new Error(`Serialized board ${boardId} stat ${statId} is truncated`);
      const dataLength = bytes.readInt16LE(statOffset + 23);
      add(statOffset + 17, 4, `board ${boardId} stat ${statId} Pascal data pointer`);
      if (!loadedFromFile) add(statOffset + 25, 8, `board ${boardId} stat ${statId} unused pointer/reserved bytes`);
      statOffset += 33;
      if (dataLength > 0) statOffset += dataLength;
      if (statOffset > boardEnd) throw new Error(`Serialized board ${boardId} stat ${statId} script exceeds its board record`);
    }
    if (statOffset !== boardEnd) {
      throw new Error(`Serialized board ${boardId} parsed through byte ${statOffset}, record ends at ${boardEnd}`);
    }
    fileOffset = boardEnd;
  }
  if (fileOffset !== bytes.length) throw new Error(`Integrated save has ${bytes.length - fileOffset} unclassified trailing byte(s)`);
  return ranges;
}

function compareSerializedSave(label, pascalSave, productionSave, loadedFromFile) {
  if (productionSave.length !== pascalSave.length) {
    throw new Error(`${label}: first serialized-byte divergence at file length: Pascal=${pascalSave.length}, production=${productionSave.length}`);
  }
  const ranges = integratedSaveNondeterministicRanges(pascalSave, loadedFromFile);
  const classifiedFields = new Map();
  for (const range of ranges) {
    for (let offset = range.start; offset < range.start + range.length; offset++) {
      if (classifiedFields.has(offset)) throw new Error(`${label}: serialized byte ${offset} was classified twice as nondeterministic`);
      classifiedFields.set(offset, range.label);
    }
  }
  let comparedBytes = 0;
  let classifiedBytes = 0;
  const classifiedDifferences = new Map();
  for (let byteIndex = 0; byteIndex < pascalSave.length; byteIndex++) {
    const nondeterministicField = classifiedFields.get(byteIndex);
    if (nondeterministicField) {
      classifiedBytes++;
      if (productionSave[byteIndex] !== pascalSave[byteIndex]) {
        classifiedDifferences.set(nondeterministicField,
          (classifiedDifferences.get(nondeterministicField) || 0) + 1);
      }
      continue;
    }
    if (productionSave[byteIndex] !== pascalSave[byteIndex]) {
      const from = Math.max(0, byteIndex - 8);
      const to = Math.min(pascalSave.length - 1, byteIndex + 8);
      const context = [];
      for (let at = from; at <= to; at++) {
        context.push(`${at}:Pascal=0x${pascalSave[at].toString(16).padStart(2, '0')},production=0x${productionSave[at].toString(16).padStart(2, '0')}`);
      }
      throw new Error(`${label}: first stable serialized-byte divergence at offset ${byteIndex}: Pascal=0x${pascalSave[byteIndex].toString(16).padStart(2, '0')}, production=0x${productionSave[byteIndex].toString(16).padStart(2, '0')}; context=[${context.join(' ')}]`);
    }
    comparedBytes++;
  }
  return { comparedBytes, classifiedBytes, classifiedDifferences };
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
  let result;
  if (scenario === 'synthetic') {
    result = instance.exports.run_integrated_gameplay_checks();
  } else {
    if (scenario === 'demo-traversal') {
      const demoBytes = fs.readFileSync(demoWorldPath);
      const demoCapacity = instance.exports.integrated_demo_world_bytes_capacity();
      if (demoBytes.length > demoCapacity) throw new Error(`DEMO.ZZT length ${demoBytes.length} exceeds production replay input capacity ${demoCapacity}`);
      new Uint8Array(instance.exports.memory.buffer,
        instance.exports.integrated_demo_world_bytes_ptr(), demoBytes.length).set(demoBytes);
      result = instance.exports.run_integrated_demo_gameplay_checks(demoBytes.length);
    } else if (scenario === 'walls-pushers-duplicators') {
      result = instance.exports.run_integrated_wall_pusher_duplicator_checks();
    } else if (scenario === 'combat-keys-pickups') {
      result = instance.exports.run_integrated_combat_key_pickup_checks();
    } else if (scenario === 'board-neighbors-passages') {
      result = instance.exports.run_integrated_board_neighbor_passage_checks();
    } else if (scenario === 'dark-room-torch-energizer-timers-death') {
      result = instance.exports.run_integrated_dark_room_torch_energizer_timers_death_checks();
    } else {
      result = instance.exports.run_integrated_script_stat_order_checks();
    }
  }
  if (result !== 0) {
    const snapshotCount = instance.exports.integrated_gameplay_snapshot_count();
    const headers = new Int16Array(instance.exports.memory.buffer,
      instance.exports.integrated_gameplay_checkpoint_headers_ptr(), snapshotCount * 2);
    const snapshotBase = instance.exports.integrated_gameplay_snapshots_ptr();
    const snapshotStride = instance.exports.integrated_gameplay_snapshot_stride();
    const snapshotLengths = new Int32Array(instance.exports.memory.buffer,
      instance.exports.integrated_gameplay_snapshot_lengths_ptr(), snapshotCount);
    const snapshots = [];
    for (let row = 0; row < snapshotCount; row++) {
      const values = new Int32Array(instance.exports.memory.buffer,
        snapshotBase + row * snapshotStride * Int32Array.BYTES_PER_ELEMENT,
        snapshotLengths[row]);
      const statCount = values[11];
      const schedulerStart = scalarNames.length + (statCount + 1) * statFieldCount;
      const tileStart = schedulerStart + schedulerFieldCount;
      const resumeTile = scenario === 'board-neighbors-passages'
        ? values[tileStart + 2 * (23 * 27 + 17)] : 0;
      snapshots.push(`${String.fromCharCode(headers[row * 2])}/${headers[row * 2 + 1]} board=${values[3]} player=${values[13]},${values[14]} tick=${values[schedulerStart]} stat=${values[schedulerStart + 1]} paused=${values[schedulerStart + 5]} input=${values[schedulerStart + 7]},${values[schedulerStart + 8]} resumeTile=${resumeTile}`);
    }
    const rawTrace = new Int16Array(instance.exports.memory.buffer,
      instance.exports.integrated_gameplay_trace_ptr(), 24 * 15);
    const trace = [];
    for (let row = 0; row < 24; row++) {
      const at = row * 15;
      const fields = Array.from(rawTrace.slice(at, at + 15));
      if (fields.some(value => value !== 0)) {
        trace.push(`${row}:stat=${fields[0]}->${fields[1]} tick=${fields[2]} boardStats=${fields[3]} gameState=${fields[4]} stack=${fields[5]} paused=${fields[6]} result=${fields[7]} dataPos=${fields[8]} tape=${fields[9]} delta=${fields[10]},${fields[11]} rightDown=${fields[12]} rightHeld=${fields[13]} leftHeld=${fields[14]}`);
      }
    }
    throw new Error(`Production integrated replay returned ${result}; captured checkpoints=[${snapshots.join('; ')}]; recent tick slots=[${trace.join('; ')}]`);
  }

  const pascalSave = fs.readFileSync(savePath);
  const productionSaveLength = instance.exports.integrated_gameplay_save_length();
  const productionSaveResult = instance.exports.integrated_gameplay_save_result();
  const productionSaveOverflow = instance.exports.integrated_gameplay_save_overflow();
  if (productionSaveResult !== 0) throw new Error(`Production integrated save returned ${productionSaveResult}; expected success`);
  if (productionSaveOverflow !== 0) throw new Error('Production integrated save exceeded its fixed output buffer');
  const productionSave = new Uint8Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_save_bytes_ptr(), productionSaveLength);
  const loadedFromFile = scenario === 'demo-traversal';

  const pascalLoadResult = Number(fs.readFileSync(loadResultPath, 'ascii').trim());
  if (pascalLoadResult !== 1) throw new Error(`Compiled Pascal WorldLoad returned ${pascalLoadResult}; expected true/success`);
  const reloadCapacity = instance.exports.integrated_gameplay_reload_input_capacity();
  if (pascalSave.length > reloadCapacity) throw new Error(`Pascal SAV length ${pascalSave.length} exceeds production reload input capacity ${reloadCapacity}`);
  const reloadInput = new Uint8Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_reload_input_bytes_ptr(), pascalSave.length);
  reloadInput.set(pascalSave);
  const productionLoadCall = instance.exports.integrated_gameplay_reload_from_pascal_save(pascalSave.length);
  const productionLoadResult = instance.exports.integrated_gameplay_reload_result();
  if (productionLoadCall !== 0 || productionLoadResult !== 0) {
    throw new Error(`Production zoo_world_load failed: call=${productionLoadCall}, result=${productionLoadResult}`);
  }
  const roundtripCall = instance.exports.integrated_gameplay_finish_roundtrip();
  if (roundtripCall !== 0) throw new Error(`Production round-trip save returned ${roundtripCall}`);
  const roundtripSaveResult = instance.exports.integrated_gameplay_roundtrip_save_result();
  const roundtripSaveOverflow = instance.exports.integrated_gameplay_roundtrip_save_overflow();
  if (roundtripSaveResult !== 0) throw new Error(`Production round-trip zoo_world_save returned ${roundtripSaveResult}`);
  if (roundtripSaveOverflow !== 0) throw new Error('Production round-trip save exceeded its fixed output buffer');
  const pascalRoundtripSave = fs.readFileSync(roundtripSavePath);
  const productionRoundtripLength = instance.exports.integrated_gameplay_roundtrip_save_length();
  const productionRoundtripSave = new Uint8Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_roundtrip_save_bytes_ptr(), productionRoundtripLength);

  const snapshotCount = instance.exports.integrated_gameplay_snapshot_count();
  if (snapshotCount !== checkpointSpecs.length) {
    const actualHeaders = new Int16Array(instance.exports.memory.buffer,
      instance.exports.integrated_gameplay_checkpoint_headers_ptr(), snapshotCount * 2);
    const observed = [];
    for (let row = 0; row < snapshotCount; row++) {
      observed.push(`${String.fromCharCode(actualHeaders[row * 2])}/${actualHeaders[row * 2 + 1]}`);
    }
    throw new Error(`Production integrated replay has ${snapshotCount} checkpoints; expected ${checkpointSpecs.length}; actual=[${observed.join(', ')}], expected=[${checkpointSpecs.map(({ kind, id }) => `${kind}/${id}`).join(', ')}]`);
  }
  const headers = new Int16Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_checkpoint_headers_ptr(), snapshotCount * 2);
  const actualBase = instance.exports.integrated_gameplay_snapshots_ptr();
  const snapshotStride = instance.exports.integrated_gameplay_snapshot_stride();
  const snapshotLengths = new Int32Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_snapshot_lengths_ptr(), snapshotCount);
  let comparedFields = 0;
  for (let row = 0; row < checkpointSpecs.length; row++) {
    const spec = checkpointSpecs[row];
    const expectedKind = spec.kind.charCodeAt(0);
    const actualKind = headers[row * 2];
    const actualId = headers[row * 2 + 1];
    if (actualKind !== expectedKind || actualId !== spec.id) {
      throw new Error(`First divergence at ${spec.name}: checkpoint identity expected=${spec.kind}/${spec.id}, actual=${String.fromCharCode(actualKind)}/${actualId}`);
    }
    const fieldCount = expectedSnapshots[row].fieldCount;
    const actualCount = snapshotLengths[row];
    if (actualCount !== fieldCount || actualCount > snapshotStride) {
      throw new Error(`First divergence at ${spec.name}: field count Pascal=${fieldCount}, production=${actualCount}, stride=${snapshotStride}`);
    }
    const expected = expectedSnapshots[row].values;
    const actual = new Int32Array(instance.exports.memory.buffer,
      actualBase + row * snapshotStride * Int32Array.BYTES_PER_ELEMENT, actualCount);
    const statCount = expected[11];
    for (let field = 0; field < fieldCount; field++) {
      // Cell-write totals are checked against the captured event stream below,
      // where a one-write drift can be reported with its exact cell/context.
      if (scenario === 'board-neighbors-passages' && field >= fieldCount - 2) continue;
      const expectedValue = expected[field];
      const observed = actual[field];
      if (observed !== expectedValue) {
        const from = Math.max(0, field - 2);
        const to = Math.min(fieldCount - 1, field + 2);
        const context = [];
        const snapshotSummary = values => {
          const liveStats = values[11] + 1;
          const scheduler = scalarNames.length + liveStats * statFieldCount;
          return {
            board: values[3], player: [values[scalarNames.length], values[scalarNames.length + 1]],
            torches: values[4], torchTicks: values[5], energizerTicks: values[6], health: values[2],
            paused: values[scheduler + 5], tick: values[scheduler], statTick: values[scheduler + 1],
            key: values[scheduler + 9], keyBufferLength: values[scheduler + 10],
          };
        };
        for (let at = from; at <= to; at++) {
          context.push(`${describeField(at, statCount, expected)}:${expected[at]}->${actual[at]}`);
        }
        const rawTrace = new Int16Array(instance.exports.memory.buffer,
          instance.exports.integrated_gameplay_trace_ptr(), 24 * 15);
        const recentTrace = [];
        for (let traceRow = 0; traceRow < 24; traceRow++) {
          const traceAt = traceRow * 15;
          const traceFields = Array.from(rawTrace.slice(traceAt, traceAt + 15));
          if (traceFields.some(value => value !== 0)) {
            recentTrace.push(`${traceRow}:tick=${traceFields[2]} stat=${traceFields[0]}->${traceFields[1]} tape=${traceFields[9]} delta=${traceFields[10]},${traceFields[11]} downR=${traceFields[12]} heldR=${traceFields[13]} heldL=${traceFields[14]}`);
          }
        }
        throw new Error(`First divergence at ${spec.name}, field ${field} (${describeField(field, statCount, expected)}): Pascal=${expectedValue}, production=${observed}; state Pascal=${JSON.stringify(snapshotSummary(expected))}, production=${JSON.stringify(snapshotSummary(actual))}; context=[${context.join(', ')}]; recent input trace=[${recentTrace.join('; ')}]`);
      }
      comparedFields++;
    }
  }

  const finalLength = snapshotLengths[snapshotCount - 1];
  const final = new Int32Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_output_ptr(), finalLength);
  const finalStatSlots = final[11] + 1;
  const schedulerStart = scalarNames.length + finalStatSlots * statFieldCount;
  const paused = final[schedulerStart + 5];
  const exitRequested = final[schedulerStart + 6];
  if (scenario === 'synthetic'
      && (final[0] !== 1 || final[1] !== 1 || final[11] !== 2 || paused !== 0 || exitRequested !== 1)) {
    throw new Error(`Integrated gameplay invariant failed: ammo=${final[0]}, gems=${final[1]}, stats=${final[11]}, paused=${paused}, exit=${exitRequested}`);
  }
  if (scenario === 'demo-traversal') {
    const playerStatStart = scalarNames.length;
    const startX = expectedSnapshots[0].values[playerStatStart];
    const startY = expectedSnapshots[0].values[playerStatStart + 1];
    const startBoard = expectedSnapshots[0].values[3];
    const traversed = checkpointSpecs.some((spec, row) => {
      if (spec.kind !== 'C') return false;
      const values = expectedSnapshots[row].values;
      return values[playerStatStart] !== startX || values[playerStatStart + 1] !== startY || values[3] !== startBoard;
    });
    if (!traversed) throw new Error(`DEMO traversal input tape produced no player movement from board ${startBoard} (${startX},${startY})`);
  }
  if (scenario === 'walls-pushers-duplicators') {
    const final = expectedSnapshots.at(-1).values;
    const statCount = final[11];
    const tileStart = scalarNames.length + (statCount + 1) * statFieldCount + schedulerFieldCount;
    const tileElementAt = (x, y) => final[tileStart + ((x * 27 + y) * 2)];
    const statField = (statId, field) => final[scalarNames.length + statId * statFieldCount + field];
    const requiredTiles = [
      [8, 5, 24, 'player pushed the boulder up to the wall'],
      [9, 5, 21, 'player-side solid wall'],
      [11, 10, 40, 'first pusher advanced through the chain'],
      [12, 10, 40, 'second pusher advanced through the chain'],
      [13, 10, 24, 'pusher moved the boulder up to the wall'],
      [14, 10, 21, 'pusher-side solid wall'],
      [19, 15, 36, 'duplicator-created object'],
      [20, 15, 12, 'duplicator remains in its source cell'],
      [21, 15, 36, 'duplicator source object remains in place'],
    ];
    if (statCount !== 5) throw new Error(`F02 coverage assertion failed: expected source plus cloned object and three mechanism stats (5 total), observed ${statCount}`);
    for (const [x, y, expected, behavior] of requiredTiles) {
      const observed = tileElementAt(x, y);
      if (observed !== expected) throw new Error(`F02 coverage assertion failed at tile (${x},${y}) for ${behavior}: expected element ${expected}, observed ${observed}`);
    }
    if (statField(0, 0) !== 7 || statField(2, 0) !== 11 || statField(3, 0) !== 12 || statField(5, 0) !== 19 || statField(5, 1) !== 15) {
      throw new Error(`F02 coverage assertion failed: expected pusher chain and appended duplicator stat order; stats=${JSON.stringify([2, 3, 4, 5].map(id => final.slice(scalarNames.length + id * statFieldCount, scalarNames.length + (id + 1) * statFieldCount)))}`);
    }
    if (final[1] < 2) throw new Error(`F02 coverage assertion failed: expected source and same-cycle cloned object scripts to execute, observed gems=${final[1]}`);
  }
  if (scenario === 'combat-keys-pickups') {
    const final = expectedSnapshots.at(-1).values;
    const statCount = final[11];
    const playerStatStart = scalarNames.length;
    const tileStart = scalarNames.length + (statCount + 1) * statFieldCount + schedulerFieldCount;
    const tileElementAt = (x, y) => final[tileStart + ((x * 27 + y) * 2)];
    if (statCount !== 1) throw new Error(`F03 coverage assertion failed: expected the player as the only remaining stat, observed Board.StatCount=${statCount}`);
    if (final[0] !== 5 || final[1] !== 1 || final[2] !== 101 || final[7] !== 10) {
      throw new Error(`F03 coverage assertion failed: ammo=${final[0]}, gems=${final[1]}, health=${final[2]}, score=${final[7]}; expected 5, 1, 101, 10`);
    }
    if (final[playerStatStart] !== 10 || final[playerStatStart + 1] !== 5) {
      throw new Error(`F03 coverage assertion failed: expected player at (10,5), observed (${final[playerStatStart]},${final[playerStatStart + 1]})`);
    }
    for (const [x, y, behavior] of [
      [6, 5, 'colored key pickup'],
      [7, 5, 'lion destroyed by the player bullet'],
      [8, 5, 'gem pickup after combat'],
      [9, 5, 'matching colored door opened'],
    ]) {
      if (tileElementAt(x, y) !== 0) {
        throw new Error(`F03 coverage assertion failed at tile (${x},${y}) for ${behavior}: expected empty, observed element ${tileElementAt(x, y)}`);
      }
    }
    if (tileElementAt(10, 5) !== 4) {
      throw new Error(`F03 coverage assertion failed at tile (10,5) after ammunition pickup: expected player element 4, observed ${tileElementAt(10, 5)}`);
    }
  }
  if (scenario === 'object-stat-order') {
    const final = expectedSnapshots.at(-1).values;
    const firstScriptCycleIndex = checkpointSpecs.findIndex(spec => spec.kind === 'C' && spec.id === 2);
    if (firstScriptCycleIndex < 0) throw new Error('F04 coverage assertion failed: cycle 2 checkpoint is missing');
    const firstScriptCycle = expectedSnapshots[firstScriptCycleIndex].values;
    const statCount = final[11];
    const statField = (statId, field) => final[scalarNames.length + statId * statFieldCount + field];
    if (statCount !== 2 || statField(1, 0) !== 20 || statField(1, 1) !== 5
        || statField(2, 0) !== 10 || statField(2, 1) !== 5) {
      throw new Error(`F04 coverage assertion failed: expected two non-player stats 1=(20,5), 2=(10,5); Board.StatCount=${statCount}, stat1=(${statField(1, 0)},${statField(1, 1)}), stat2=(${statField(2, 0)},${statField(2, 1)})`);
    }
    if (firstScriptCycle[0] !== 1 || firstScriptCycle[1] !== 1 || final[0] !== 1 || final[1] !== 1) {
      throw new Error(`F04 coverage assertion failed: stat-1 #SET/#GIVE must be visible to stat-2 #IF in its first script cycle and persist; C2 ammo/gems=${firstScriptCycle[0]}/${firstScriptCycle[1]}, final ammo/gems=${final[0]}/${final[1]}`);
    }
  }
  if (scenario === 'board-neighbors-passages') {
    const boardEntryIndex = checkpointSpecs.findIndex(spec => spec.kind === 'T' && spec.id === 2);
    const passageIndex = checkpointSpecs.findIndex(spec => spec.kind === 'T' && spec.id === 3);
    if (boardEntryIndex < 0 || passageIndex < 0) {
      throw new Error('F05 coverage assertion failed: expected board-entry T2 and passage T3 checkpoints');
    }
    const boardEntry = expectedSnapshots[boardEntryIndex].values;
    const passage = expectedSnapshots[passageIndex].values;
    const final = expectedSnapshots.at(-1).values;
    const tileStartFor = values => scalarNames.length + (values[11] + 1) * statFieldCount + schedulerFieldCount;
    const tileElementAt = (values, x, y) => values[tileStartFor(values) + ((x * 27 + y) * 2)];
    const statField = (values, statId, field) => values[scalarNames.length + statId * statFieldCount + field];
    const passageSchedulerStart = scalarNames.length + (passage[11] + 1) * statFieldCount;
    if (boardEntry[3] !== 1 || statField(boardEntry, 0, 0) !== 60 || statField(boardEntry, 0, 1) !== 5) {
      throw new Error(`F05 coverage assertion failed at T2: expected board 1 immediately after BoardChange loaded its west-edge start (60,5), observed board=${boardEntry[3]}, player=(${statField(boardEntry, 0, 0)},${statField(boardEntry, 0, 1)})`);
    }
    if (passage[3] !== 2 || statField(passage, 0, 0) !== 22 || statField(passage, 0, 1) !== 17
        || passage[passageSchedulerStart + 5] !== 1) {
      throw new Error(`F05 coverage assertion failed at T3: expected board 2 passage destination (22,17) with paused state, observed board=${passage[3]}, player=(${statField(passage, 0, 0)},${statField(passage, 0, 1)}), paused=${passage[passageSchedulerStart + 5]}`);
    }
    if (final[3] !== 2 || statField(final, 0, 0) !== 23 || statField(final, 0, 1) !== 17
        || tileElementAt(final, 20, 15) !== 11 || tileElementAt(final, 22, 17) !== 11
        || tileElementAt(final, 30, 12) !== 0) {
      throw new Error(`F05 coverage assertion failed: expected last matching passage at (22,17), one resumed step to (23,17), earlier match retained at (20,15), and cleared board spawn (30,12); final board=${final[3]}, player=(${statField(final, 0, 0)},${statField(final, 0, 1)})`);
    }
  }
  const expectedSound = parseSoundEvents(soundPath);
  if (expectedSound.overflow !== 0) throw new Error('Pascal integrated sound-event capture overflowed its fixed 64-event buffer');
  const actualSoundCount = instance.exports.integrated_gameplay_sound_event_count();
  const actualSoundOverflow = instance.exports.integrated_gameplay_sound_event_overflow();
  const actualSoundStride = instance.exports.integrated_gameplay_sound_event_stride();
  const expectedSoundStride = 516;
  if (actualSoundOverflow !== 0) throw new Error('Production integrated sound-event capture overflowed or saw an invalid event length');
  if (actualSoundStride !== expectedSoundStride) throw new Error(`Production sound-event stride ${actualSoundStride}; expected ${expectedSoundStride}`);
  if (actualSoundCount !== expectedSound.count) {
    throw new Error(`First sound-event divergence at event count: Pascal=${expectedSound.count}, production=${actualSoundCount}; Pascal context=${JSON.stringify(expectedSound.events.slice(-2))}`);
  }
  const actualSoundBase = instance.exports.integrated_gameplay_sound_events_ptr();
  const actualSound = new Int32Array(instance.exports.memory.buffer,
    actualSoundBase, actualSoundCount * actualSoundStride);
  const soundFieldNames = ['priority', 'request length', 'result priority', 'result position', 'result length', 'result playing'];
  let soundComparedFields = 0;
  for (let eventIndex = 0; eventIndex < expectedSound.count; eventIndex++) {
    const expected = expectedSound.events[eventIndex];
    const base = eventIndex * actualSoundStride;
    const actualRequestLength = actualSound[base + 1];
    const actualResultLength = actualSound[base + 259];
    const scalarExpected = [expected.priority, expected.requestLength, expected.resultPriority,
      expected.resultPosition, expected.resultLength, expected.resultPlaying];
    const scalarActual = [actualSound[base], actualRequestLength, actualSound[base + 257],
      actualSound[base + 258], actualResultLength, actualSound[base + 260]];
    for (let fieldIndex = 0; fieldIndex < scalarExpected.length; fieldIndex++) {
      if (scalarActual[fieldIndex] !== scalarExpected[fieldIndex]) {
        const from = Math.max(0, eventIndex - 1);
        const to = Math.min(expectedSound.count - 1, eventIndex + 1);
        throw new Error(`First sound-event divergence at event ${eventIndex}, ${soundFieldNames[fieldIndex]}: Pascal=${scalarExpected[fieldIndex]}, production=${scalarActual[fieldIndex]}; surrounding Pascal events=${JSON.stringify(expectedSound.events.slice(from, to + 1))}`);
      }
      soundComparedFields++;
    }
    const byteComparisons = [
      { name: 'request byte', expected: expected.requestBytes, start: base + 2 },
      { name: 'queued byte', expected: expected.resultBytes, start: base + 261 },
    ];
    for (const bytes of byteComparisons) {
      for (let byteIndex = 0; byteIndex < bytes.expected.length; byteIndex++) {
        const observed = actualSound[bytes.start + byteIndex];
        if (observed !== bytes.expected[byteIndex]) {
          const from = Math.max(0, eventIndex - 1);
          const to = Math.min(expectedSound.count - 1, eventIndex + 1);
          throw new Error(`First sound-event divergence at event ${eventIndex}, ${bytes.name} ${byteIndex}: Pascal=${bytes.expected[byteIndex]}, production=${observed}; surrounding Pascal events=${JSON.stringify(expectedSound.events.slice(from, to + 1))}`);
        }
        soundComparedFields++;
      }
    }
  }
  const expectedCells = parseCellWrites(cellPath);
  if (expectedCells.overflow !== 0) throw new Error('Pascal integrated cell-write capture overflowed its fixed 8192-event buffer');
  if (scenario === 'dark-room-torch-energizer-timers-death') {
    const final = expectedSnapshots.at(-1).values;
    const cycleSnapshots = checkpointSpecs
      .map((spec, row) => ({ spec, values: expectedSnapshots[row].values }))
      .filter(checkpoint => checkpoint.spec.kind === 'C');
    const torchPickup = cycleSnapshots.some(checkpoint => checkpoint.values[4] === 1);
    const torchLit = cycleSnapshots.some(checkpoint => checkpoint.values[4] === 0 && checkpoint.values[5] > 0);
    const energizerActivated = cycleSnapshots.some(checkpoint => checkpoint.values[6] > 0);
    const energizerExpired = energizerActivated && cycleSnapshots.some(checkpoint => checkpoint.values[6] === 0);
    // Cell (14,4) maps to board tile (15,5), in the player's dark redraw
    // rectangle but beyond the lit radius after moving to (6,5).
    const darkSample = expectedCells.events.some(event => event.x === 14 && event.y === 4
      && event.color === 0x07 && event.character === 0xB0);
    if (!torchPickup || !torchLit || !energizerActivated || !energizerExpired
        || final[2] !== 0 || final[5] <= 0 || final[6] !== 0 || !darkSample) {
      throw new Error(`F06 coverage assertion failed: torchPickup=${torchPickup}, torchLit=${torchLit}, energizerActivated=${energizerActivated}, energizerExpired=${energizerExpired}, finalHealth=${final[2]}, finalTorchTicks=${final[5]}, finalEnergizerTicks=${final[6]}, darkSampleDrawn=${darkSample}`);
    }
  }
  const actualCellCount = instance.exports.integrated_gameplay_cell_event_count();
  const actualCellOverflow = instance.exports.integrated_gameplay_cell_event_overflow();
  const actualCellStride = instance.exports.integrated_gameplay_cell_event_stride();
  if (actualCellOverflow !== 0) throw new Error(`Production integrated cell-change capture overflowed its fixed buffer at ${actualCellCount} records (idempotent writes filtered: ${instance.exports.integrated_gameplay_cell_idempotent_write_count()})`);
  if (actualCellStride !== 4) throw new Error(`Production cell-write stride ${actualCellStride}; expected 4`);
  const actualCellBase = instance.exports.integrated_gameplay_cell_events_ptr();
  const actualCells = new Int32Array(instance.exports.memory.buffer,
    actualCellBase, Math.min(actualCellCount, expectedCells.count) * actualCellStride);
  const actualCellContextStride = instance.exports.integrated_gameplay_cell_context_stride();
  const actualCellContexts = new Int32Array(instance.exports.memory.buffer,
    instance.exports.integrated_gameplay_cell_context_events_ptr(),
    Math.min(actualCellCount, expectedCells.count) * actualCellContextStride);
  const actualIdempotentWriteCount = instance.exports.integrated_gameplay_cell_idempotent_write_count();
  const cellFieldNames = ['x', 'y', 'attribute', 'CP437 byte'];
  let cellComparedFields = 0;
  for (let eventIndex = 0; eventIndex < expectedCells.count; eventIndex++) {
    const expected = expectedCells.events[eventIndex];
    const expectedFields = [expected.x, expected.y, expected.color, expected.character];
    const base = eventIndex * actualCellStride;
    for (let fieldIndex = 0; fieldIndex < expectedFields.length; fieldIndex++) {
      const observed = actualCells[base + fieldIndex];
      if (observed !== expectedFields[fieldIndex]) {
        const from = Math.max(0, eventIndex - 2);
        const to = Math.min(expectedCells.count - 1, eventIndex + 2);
        const actualContext = [];
        for (let at = from; at <= to; at++) {
          const pos = at * actualCellStride;
          const contextPos = at * actualCellContextStride;
          actualContext.push({
            x: actualCells[pos], y: actualCells[pos + 1], color: actualCells[pos + 2], character: actualCells[pos + 3],
            board: actualCellContexts[contextPos], player: [actualCellContexts[contextPos + 1], actualCellContexts[contextPos + 2]],
            paused: actualCellContexts[contextPos + 3], pauseBlink: actualCellContexts[contextPos + 4],
            callState: actualCellContexts[contextPos + 5], stat: actualCellContexts[contextPos + 6], tick: actualCellContexts[contextPos + 7],
          });
        }
        throw new Error(`First cell-write divergence at event ${eventIndex}, ${cellFieldNames[fieldIndex]}: Pascal=${expectedFields[fieldIndex]}, production=${observed}; surrounding Pascal=${JSON.stringify(expectedCells.events.slice(from, to + 1))}; production=${JSON.stringify(actualContext)}`);
      }
      cellComparedFields++;
    }
  }
  if (actualCellCount !== expectedCells.count) {
    const from = Math.max(0, Math.min(actualCellCount, expectedCells.count) - 3);
    const actualTail = [];
    for (let eventIndex = from; eventIndex < actualCellCount; eventIndex++) {
      const base = eventIndex * actualCellStride;
      const event = new Int32Array(instance.exports.memory.buffer,
        actualCellBase + base * Int32Array.BYTES_PER_ELEMENT, actualCellStride);
      actualTail.push({ x: event[0], y: event[1], color: event[2], character: event[3] });
    }
    throw new Error(`First cell-write divergence at event count: Pascal=${expectedCells.count}, production=${actualCellCount}; Pascal tail=${JSON.stringify(expectedCells.events.slice(-3))}; production tail begins at ${from} (${actualCellCount - from} events)`);
  }

  // Compare final serialized bytes only after the earlier state-transition,
  // sound, and cell-write checks. This preserves first-divergence ordering:
  // a final save mismatch must not hide an earlier gameplay mismatch.
  const generatedSaveComparison = compareSerializedSave(
    loadedFromFile ? 'Generated DEMO-derived SAV' : 'Generated fresh-world SAV',
    pascalSave, productionSave, loadedFromFile);
  const roundtripSaveComparison = compareSerializedSave(
    'Loaded-file round-trip SAV', pascalRoundtripSave, productionRoundtripSave, true);

  const checkpointFieldCounts = expectedSnapshots.map(snapshot => snapshot.fieldCount);
  const classifiedDiffSummary = [...generatedSaveComparison.classifiedDifferences.entries()]
    .map(([field, count]) => `${field} (${count} differing byte${count === 1 ? '' : 's'})`)
    .join('; ') || 'none';
  const generatedLabel = loadedFromFile ? 'DEMO-derived SAV' : 'fresh-world SAV';
  const cellWriteSummary = ['board-neighbors-passages', 'dark-room-torch-energizer-timers-death'].includes(scenario)
    ? `${expectedCells.count} playfield cell changes matched (${cellComparedFields} event fields); ${expectedCells.idempotentWriteCount} Pascal and ${actualIdempotentWriteCount} production identical-value redraws were reported separately`
    : `${expectedCells.count} playfield cell writes matched (${cellComparedFields} event fields)`;
  console.log(`Integrated gameplay replay (${scenario}): ${checkpointSpecs.length} checkpoints (${checkpointFieldCounts.join(', ')} fields) matched (${comparedFields} comparisons); ${expectedSound.count} logical sound-queue events matched (${soundComparedFields} event fields); ${cellWriteSummary}; generated ${generatedLabel} matched ${generatedSaveComparison.comparedBytes} stable bytes with ${generatedSaveComparison.classifiedBytes} undefined/pointer positions classified (${classifiedDiffSummary}); loaded-file round-trip SAV matched ${roundtripSaveComparison.comparedBytes} preserved bytes with ${roundtripSaveComparison.classifiedBytes} process-pointer positions classified; Pascal WorldLoad=true matched production zoo_world_load=0; first-divergence reports identify checkpoint/event, field, tile, cell, serialized byte, and surrounding values`);
})().catch(error => { console.error(error); process.exitCode = 1; });
