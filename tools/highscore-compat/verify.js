const fs = require('node:fs');
const path = require('node:path');

const TABLE_BYTES = 1590;
const ENTRY_BYTES = 53;
const COUNT = 30;
const NAME_BYTES = 50;
const SCORE_OFFSET = 51;

function requireEqual(actual, expected, message) {
  if (!actual.equals(expected)) {
    let offset = 0;
    while (offset < Math.min(actual.length, expected.length) && actual[offset] === expected[offset]) offset++;
    throw new Error(`${message}; first byte mismatch at ${offset} (actual length ${actual.length}, expected ${expected.length})`);
  }
}

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

async function main() {
  const [wasmPath, ...args] = process.argv.slice(2);
  if (!wasmPath) throw new Error('Expected a shared high-score WASM harness.');
  const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath));
  const api = instance.exports;
  const memory = new Uint8Array(api.memory.buffer);
  const read = (ptr, length) => Buffer.from(memory.slice(ptr, ptr + length));
  const write = (ptr, bytes) => memory.set(bytes, ptr);
  const tablePtr = api.hs_table_ptr();
  const inputPtr = api.hs_input_ptr();
  const namePtr = api.hs_name_ptr();
  const outputPtr = api.hs_output_ptr();

  const init = () => assert(api.hs_init() === 0, 'Shared table initialization failed.');
  const load = (bytes, succeeded = true) => {
    assert(bytes.length <= 4096, 'High-score fixture exceeds harness input capacity.');
    write(inputPtr, bytes);
    return api.hs_load(bytes.length, succeeded ? 1 : 0);
  };
  const save = () => {
    assert(api.hs_save() === 0, 'Shared table save failed.');
    return read(outputPtr, TABLE_BYTES);
  };
  const tableBytes = () => read(tablePtr, TABLE_BYTES);
  const writeOut = (file, bytes) => fs.writeFileSync(file, bytes);
  const seededScores = () => {
    const bytes = Buffer.alloc(TABLE_BYTES);
    for (let i = 0; i < COUNT; i++) {
      const offset = i * ENTRY_BYTES;
      bytes[offset] = 50;
      for (let j = 0; j < NAME_BYTES; j++) bytes[offset + 1 + j] = 65 + ((i * 7 + j) % 26);
      bytes.writeInt16LE((i * 101) - 1515, offset + SCORE_OFFSET);
    }
    return bytes;
  };

  init();
  const blank = Buffer.alloc(TABLE_BYTES);
  for (let i = 0; i < COUNT; i++) blank.writeInt16LE(-1, i * ENTRY_BYTES + SCORE_OFFSET);
  requireEqual(tableBytes(), blank, 'Initialized table differs from Pascal defaults');
  console.log('shared table initialization: exact 1,590-byte defaults match');

  const staleSeed = seededScores();
  init();
  assert(load(staleSeed) === 0, 'Shared implementation rejected a complete .HI record.');
  requireEqual(tableBytes(), staleSeed, 'Complete .HI load changed raw Pascal record bytes');
  requireEqual(save(), staleSeed, 'Shared .HI save changed raw Pascal record bytes');
  const extended = Buffer.concat([staleSeed, Buffer.alloc(7, 0xa5)]);
  init();
  assert(load(extended) === 0, 'Shared implementation rejected trailing .HI bytes.');
  requireEqual(save(), staleSeed, 'Shared loader did not ignore bytes after the one Pascal record');

  const malformed = Buffer.from(staleSeed);
  malformed[0] = 0xff;
  init();
  assert(load(malformed) === 0, 'Shared implementation rejected a noncanonical Pascal string length.');
  requireEqual(save(), malformed, 'Raw load normalized a noncanonical Pascal string length');

  init();
  assert(load(staleSeed) === 0, 'Could not seed table for Pascal reset semantics.');
  assert(load(staleSeed.subarray(0, TABLE_BYTES - 1), false) !== 0,
    'Short typed-file read did not surface a load error.');
  const resetFromSeed = Buffer.from(staleSeed);
  for (let i = 0; i < COUNT; i++) {
    const offset = i * ENTRY_BYTES;
    resetFromSeed[offset] = 0;
    resetFromSeed.writeInt16LE(-1, offset + SCORE_OFFSET);
  }
  requireEqual(save(), resetFromSeed, 'Failed load did not reset lengths and scores while retaining stale payload bytes');
  console.log('shared load/save: exact records, trailing bytes, raw lengths, short-read reset and stale tails pass');

  const readScore = (bytes, index) => bytes.readInt16LE(index * ENTRY_BYTES + SCORE_OFFSET);
  const expectedPosition = (bytes, score) => {
    let position = 0;
    while (position < COUNT && score < readScore(bytes, position)) position++;
    return score > 0 && position < COUNT ? position : -1;
  };
  const assignShortString = (bytes, offset, value) => {
    const length = Math.min(NAME_BYTES, value.length);
    bytes[offset] = length;
    value.copy(bytes, offset + 1, 0, length);
  };
  const expectedInsert = (bytes, position, score, value) => {
    for (let i = COUNT - 1; i > position; i--) {
      bytes.copyWithin(i * ENTRY_BYTES, (i - 1) * ENTRY_BYTES, i * ENTRY_BYTES);
    }
    const offset = position * ENTRY_BYTES;
    bytes.writeInt16LE(score, offset + SCORE_OFFSET);
    assignShortString(bytes, offset, Buffer.from('-- You! --', 'ascii'));
    assignShortString(bytes, offset, value);
  };
  let randomState = 0x5a17c9e3;
  const nextByte = () => {
    randomState ^= randomState << 13;
    randomState ^= randomState >>> 17;
    randomState ^= randomState << 5;
    return randomState & 0xff;
  };
  const lengthChoices = [0, 1, 9, 34, 50, 51, 127, 255];
  const scoreChoices = [-32768, -1, 0, 1, 2, 500, 32767];
  const nameChoices = [
    Buffer.alloc(0), Buffer.from('T', 'ascii'), Buffer.from([0x42, 0x94, 0x62]),
    Buffer.from('Kerry 49', 'ascii'), Buffer.alloc(55, 0x4e),
  ];
  for (let trial = 0; trial < 256; trial++) {
    const fixture = Buffer.alloc(TABLE_BYTES);
    for (let i = 0; i < fixture.length; i++) fixture[i] = nextByte();
    for (let i = 0; i < COUNT; i++) {
      const offset = i * ENTRY_BYTES;
      fixture[offset] = lengthChoices[(trial + i) % lengthChoices.length];
      fixture.writeInt16LE((nextByte() << 8 | nextByte()) - 32768, offset + SCORE_OFFSET);
    }
    const score = trial < scoreChoices.length ? scoreChoices[trial] : (nextByte() * 128);
    const value = nameChoices[trial % nameChoices.length];
    const position = expectedPosition(fixture, score);
    init();
    assert(load(fixture) === 0, `Randomized trial ${trial} failed to load.`);
    assert(api.hs_insertion_position(score) === position,
      `Insertion rank diverged from the Pascal source model at trial ${trial}.`);
    const expected = Buffer.from(fixture);
    if (position >= 0) {
      expectedInsert(expected, position, score, value);
      write(namePtr, value);
      assert(api.hs_insert(position, score, value.length) === 0,
        `Shared insertion rejected trial ${trial}.`);
    }
    requireEqual(save(), expected, `Shared insertion differs from source model at trial ${trial}`);
  }
  console.log('256 full-table source-model insertion rows: all positions, aliases, ties and stale tails pass');

  let differentialCount = 0;
  if (args[0] && args[0] !== '-') {
    const pascalHi = fs.readFileSync(args[0]);
    assert(pascalHi.length === TABLE_BYTES, 'Turbo Pascal save fixture is not 1,590 bytes.');
    init();
    assert(load(pascalHi) === 0, 'Shared loader rejected Turbo Pascal save fixture.');
    requireEqual(save(), pascalHi, 'Shared .HI serializer differs from the Turbo Pascal save fixture');
    writeOut(args[1], save());
    differentialCount++;
    console.log('Turbo Pascal HighScoresSave: all 1,590 bytes match shared LibZoo');
  }

  if (args[2] && args[2] !== '-') {
    const directory = args[2];
    const seed = fs.readFileSync(path.join(directory, 'HISEED.HI'));
    assert(seed.length === TABLE_BYTES, 'Turbo Pascal load seed has the wrong record size.');
    const cases = ['HIMISS', 'HIEMPT', 'HIC001', 'HIC052', 'HIC053', 'HIC1589', 'HIFULL', 'HILONG'];
    for (const name of cases) {
      init();
      assert(load(seed) === 0, `Could not seed Pascal load case ${name}.`);
      const inputPath = path.join(directory, `${name}.HI`);
      const exists = fs.existsSync(inputPath);
      const record = exists ? fs.readFileSync(inputPath) : Buffer.alloc(0);
      const success = exists && record.length >= TABLE_BYTES;
      const status = load(record, success);
      assert(success ? status === 0 : status !== 0,
        `${name} load status does not match Pascal Reset/Read result.`);
      const actual = save();
      const expected = fs.readFileSync(path.join(directory, `${name}.OUT`));
      requireEqual(actual, expected, `Turbo Pascal HighScoresLoad ${name} differs from shared LibZoo`);
      writeOut(path.join(args[3], `load-${name}.HI`), actual);
      differentialCount++;
      console.log(`Turbo Pascal HighScoresLoad ${name}: all 1,590 bytes match shared LibZoo`);
    }
  }

  if (args[4] && args[4] !== '-') {
    const directory = args[4];
    const outputDirectory = args[5];
    const scenarios = [
      ['HITOP', 32767, ''], ['HITIE', 23000, ''], ['HIBOT', 15500, ''],
      ['HICUT', 15499, ''], ['HIZERO', 0, ''], ['HINEG', -32768, ''], ['HINAME', 25000, 'KERRY'],
    ];
    for (const [name, score, enteredName] of scenarios) {
      const inputPath = path.join(directory, `${name}.IN`);
      const expectedPath = path.join(directory, `${name}.OUT`);
      const input = fs.readFileSync(inputPath);
      const expected = fs.readFileSync(expectedPath);
      assert(input.length === TABLE_BYTES && expected.length === TABLE_BYTES,
        `${name} Pascal insertion fixture has the wrong size.`);
      init();
      assert(load(input) === 0, `${name} fixture load failed.`);
      const position = api.hs_insertion_position(score);
      if (position >= 0) {
        const nameBytes = Buffer.from(enteredName, 'ascii');
        write(namePtr, nameBytes);
        assert(api.hs_insert(position, score, nameBytes.length) === 0,
          `${name} shared insertion failed.`);
      }
      const actual = save();
      requireEqual(actual, expected, `Turbo Pascal HighScoresAdd ${name} differs from shared LibZoo`);
      writeOut(path.join(outputDirectory, `add-${name}.HI`), actual);
      const saved = path.join(directory, `${name}.HI`);
      if (position >= 0) {
        assert(fs.existsSync(saved), `${name} accepted score did not save in Pascal.`);
        requireEqual(fs.readFileSync(saved), expected, `${name} saved .HI differs from the direct Pascal table`);
      } else {
        assert(!fs.existsSync(saved), `${name} rejected score unexpectedly saved a Pascal .HI file.`);
      }
      differentialCount++;
      console.log(`Turbo Pascal HighScoresAdd ${name}: all 1,590 bytes match shared LibZoo`);
    }
  }

  console.log(`Direct Pascal high-score comparisons: ${differentialCount}; source-derived model rows: 256; unexplained mismatches: 0`);
}

main().catch(error => {
  console.error(error.stack || error.message || String(error));
  process.exitCode = 1;
});
