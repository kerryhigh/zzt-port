const fs = require('node:fs');

const pascalPath = process.argv[2];
const wasmPath = process.argv[3];
const titleQuitPascalPath = process.argv[4] || null;
const endPlayLoopPascalPath = process.argv[5] || null;
const demoWorldPath = process.argv[6] || null;
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: compare-player-abort.js <ABORT.OUT> <integrated_gameplay.wasm> [TITLEQUIT.OUT [ABORTLP.OUT DEMO.ZZT]]');
}

const expected = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/).map((line, lineIndex) => {
  const values = line.trim().split(/\s+/).map(Number);
  if (values.length !== 5 || values.some(value => !Number.isInteger(value))) {
    throw new Error(`Malformed Pascal end-play row ${lineIndex + 1}: ${line}`);
  }
  return values;
});
if (expected.length !== 7) throw new Error(`Pascal end-play replay emitted ${expected.length} rows; expected 7`);

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
        const next = malloc(length);
        if (next && instance) new Uint8Array(instance.exports.memory.buffer).copyWithin(next, pointer, pointer + oldLength);
        return next;
      };
      return () => 0;
    },
  });
  instance = await WebAssembly.instantiate(module, { env });
  const result = instance.exports.run_integrated_end_play_checks();
  if (result !== 0) throw new Error(`Production end-play replay returned ${result}`);
  const rows = instance.exports.integrated_end_play_output_rows();
  if (rows !== expected.length) throw new Error(`Production end-play replay emitted ${rows} rows; expected ${expected.length}`);
  const pointer = instance.exports.integrated_end_play_output_ptr();
  const actual = new Int32Array(instance.exports.memory.buffer, pointer, rows * 5);
  for (let row = 0; row < rows; row++) {
    for (let field = 0; field < 5; field++) {
      const got = actual[row * 5 + field];
      const want = expected[row][field];
      if (got !== want) {
        const names = ['case', 'exit requested', 'key pressed', 'paused', 'health'];
        throw new Error(`Player abort first difference at row ${row + 1}, ${names[field]}: Pascal=${want}, production=${got}`);
      }
    }
  }
  console.log('Player abort decision: seven direct Pascal cases match shared engine state (Yes/No/Escape at both pause states, plus the no-prompt death path).');

  if (titleQuitPascalPath) {
    const titleExpected = fs.readFileSync(titleQuitPascalPath, 'ascii').trim().split(/\r?\n/).map((line, lineIndex) => {
      const values = line.trim().split(/\s+/).map(Number);
      if (values.length !== 2 || values.some(value => !Number.isInteger(value))) {
        throw new Error(`Malformed Pascal title-quit row ${lineIndex + 1}: ${line}`);
      }
      return values;
    });
    if (titleExpected.length !== 3) throw new Error(`Pascal title-quit replay emitted ${titleExpected.length} rows; expected 3`);
    const titleResult = instance.exports.run_integrated_title_quit_checks();
    if (titleResult !== 0) throw new Error(`Production title-quit replay returned ${titleResult}`);
    const titleRows = instance.exports.integrated_title_quit_output_rows();
    if (titleRows !== titleExpected.length) throw new Error(`Production title-quit replay emitted ${titleRows} rows; expected ${titleExpected.length}`);
    const titlePointer = instance.exports.integrated_title_quit_output_ptr();
    const titleActual = new Int32Array(instance.exports.memory.buffer, titlePointer, titleRows * 2);
    for (let row = 0; row < titleRows; row++) {
      for (let field = 0; field < 2; field++) {
        const got = titleActual[row * 2 + field];
        const want = titleExpected[row][field];
        if (got !== want) {
          const name = field === 0 ? 'case' : 'GameTitleExitRequested';
          throw new Error(`Title quit first difference at row ${row + 1}, ${name}: Pascal=${want}, production=${got}`);
        }
      }
    }
    console.log('Title quit decision: direct Pascal SidebarPromptYesNo results for Yes/No/Escape match the shared engine exit flag.');
  }
  if (endPlayLoopPascalPath || demoWorldPath) {
    if (!endPlayLoopPascalPath || !demoWorldPath) {
      throw new Error('The real-loop abort differential requires both ABORTLP.OUT and DEMO.ZZT.');
    }
    const loopExpected = fs.readFileSync(endPlayLoopPascalPath, 'ascii').trim().split(/\s+/).map(Number);
    const loopNames = [
      'exit requested', 'paused', 'key pressed', 'key buffer count', 'current tick',
      'current stat tick', 'tick duration', 'tick counter', 'current board',
      'return board', 'health', 'ammo', 'save flag', 'stat count', 'player x',
      'player y', 'player tile element', 'player tile color', 'sound blocking',
      'sound buffer position', 'sound buffer length', 'sound priority',
    ];
    if (loopExpected.length !== loopNames.length || loopExpected.some(value => !Number.isInteger(value))) {
      throw new Error(`Malformed Pascal accepted-abort loop capture: ${loopExpected.length} fields, expected ${loopNames.length}.`);
    }
    const world = fs.readFileSync(demoWorldPath);
    const capacity = instance.exports.integrated_demo_world_bytes_capacity();
    if (world.length > capacity) throw new Error(`DEMO.ZZT exceeds production harness capacity: ${world.length}/${capacity}`);
    const worldPointer = instance.exports.integrated_demo_world_bytes_ptr();
    new Uint8Array(instance.exports.memory.buffer, worldPointer, world.length).set(world);
    const loopResult = instance.exports.run_integrated_end_play_loop_check(world.length);
    if (loopResult !== 0) throw new Error(`Production accepted-abort loop replay returned ${loopResult}`);
    const count = instance.exports.integrated_end_play_loop_output_count();
    if (count !== loopExpected.length) throw new Error(`Production accepted-abort loop emitted ${count} fields; expected ${loopExpected.length}`);
    const pointer = instance.exports.integrated_end_play_loop_output_ptr();
    const loopActual = new Int32Array(instance.exports.memory.buffer, pointer, count);
    for (let field = 0; field < count; field++) {
      if (loopExpected[field] !== loopActual[field]) {
        const from = Math.max(0, field - 2);
        const to = Math.min(count - 1, field + 2);
        const context = [];
        for (let near = from; near <= to; near++) {
          context.push({ field: loopNames[near], pascal: loopExpected[near], production: loopActual[near] });
        }
        throw new Error(`Accepted player-abort loop first difference at field ${field} (${loopNames[field]}): Pascal=${loopExpected[field]}, production=${loopActual[field]}; nearby=${JSON.stringify(context)}`);
      }
    }
    console.log(`Accepted player-abort loop: actual Pascal GamePlayLoop return matched production zoo_tick exit boundary across ${count}/${count} player/world/scheduler/sound fields.`);
  }
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
