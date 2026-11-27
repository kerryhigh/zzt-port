const fs = require('node:fs');
const path = require('node:path');

const stageRoot = path.resolve(process.argv[2]);
const wasmPath = path.resolve(process.argv[3]);

function readPascalSummary(name) {
  const filePath = path.join(stageRoot, name);
  if (!fs.existsSync(filePath)) throw new Error(`Missing direct Pascal state output: ${filePath}`);
  const line = fs.readFileSync(filePath, 'utf8').replace(/\r/g, '').trim();
  const match = /^board (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)$/.exec(line);
  if (!match) throw new Error(`${name}: malformed BRDXFER summary: ${line}`);
  const values = match.slice(1).map(Number);
  return {
    nameLength: values[0],
    maxShots: values[1],
    statCount: values[2],
    centerElement: values[3],
    centerColor: values[4],
    playerX: values[5],
    playerY: values[6],
    worldBoardLength: values[7],
    neighbors: values.slice(8, 12),
  };
}

function parseProductionBoard(bytes) {
  if (bytes.length < 51 + 86 + 2) throw new Error(`production board body is too short (${bytes.length} bytes)`);
  const nameLength = Math.min(bytes[0], 50);
  const name = Buffer.from(bytes.subarray(1, 1 + nameLength));
  let position = 51;
  let cells = 0;
  let centerElement = -1;
  let centerColor = -1;
  while (cells < 60 * 25) {
    if (position + 3 > bytes.length) throw new Error(`truncated RLE at tile ${cells + 1}`);
    const count = bytes[position];
    const runLength = count === 0 ? 256 : count;
    const element = bytes[position + 1];
    const color = bytes[position + 2];
    position += 3;
    const end = Math.min(60 * 25, cells + runLength);
    const target = 11 * 60 + 29; // Pascal probe's Tile[30][12], zero-based in row-major order.
    if (target >= cells && target < end) {
      centerElement = element;
      centerColor = color;
    }
    cells = end;
  }
  const infoOffset = position;
  if (infoOffset + 88 > bytes.length) throw new Error('board info/stat count is truncated');
  const statCount = bytes.readInt16LE(infoOffset + 86);
  position = infoOffset + 88;
  if (position + 33 > bytes.length) throw new Error('player stat record is truncated');
  return {
    nameLength,
    name,
    maxShots: bytes[infoOffset],
    neighbors: Array.from(bytes.subarray(infoOffset + 2, infoOffset + 6)),
    statCount,
    centerElement,
    centerColor,
    playerX: bytes[position],
    playerY: bytes[position + 1],
  };
}

function compare(label, expected, actual, fields) {
  for (const field of fields) {
    if (expected[field] !== actual[field]) {
      throw new Error(`${label}: ${field} differs; expected=${expected[field]}, production=${actual[field]}`);
    }
  }
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {});
  const { memory, input_ptr, output_ptr, output_len, run_editor_brd_transfer } = instance.exports;
  if (typeof run_editor_brd_transfer !== 'function') throw new Error('Production serializer harness lacks run_editor_brd_transfer.');

  const cases = [
    {
      id: 'short-payload',
      input: Buffer.from([0x04, 0x00, 0x10, 0x20]),
      expectedOutcome: 2,
      expected: readPascalSummary('BRDXFER.OUT'),
      evidence: 'DP',
      fields: ['nameLength', 'maxShots', 'statCount', 'centerElement', 'centerColor', 'playerX', 'playerY', 'worldBoardLength'],
    },
    {
      id: 'zero-byte-payload',
      input: Buffer.from([0x04, 0x00]),
      expectedOutcome: 2,
      expected: readPascalSummary('PAYZERO.OUT'),
      evidence: 'DP',
      fields: ['nameLength', 'maxShots', 'statCount', 'centerElement', 'centerColor', 'playerX', 'playerY', 'worldBoardLength'],
    },
    {
      id: 'one-byte-payload',
      input: Buffer.from([0x04, 0x00, 0x10]),
      expectedOutcome: 2,
      expected: readPascalSummary('PAYONE.OUT'),
      evidence: 'DP',
      fields: ['nameLength', 'maxShots', 'statCount', 'centerElement', 'centerColor', 'playerX', 'playerY', 'worldBoardLength'],
    },
    {
      id: 'three-byte-payload',
      input: Buffer.from([0x04, 0x00, 0x10, 0x20, 0x30]),
      expectedOutcome: 2,
      expected: readPascalSummary('PAYTHREE.OUT'),
      evidence: 'DP',
      fields: ['nameLength', 'maxShots', 'statCount', 'centerElement', 'centerColor', 'playerX', 'playerY', 'worldBoardLength'],
    },
    {
      id: 'empty-header-controlled-reopen',
      input: Buffer.alloc(0),
      expectedOutcome: 1,
      expected: {
        ...readPascalSummary('EMPTY.OUT'),
        name: Buffer.from('BEFORE', 'latin1'),
      },
      evidence: 'DP',
      fields: ['nameLength', 'maxShots', 'statCount', 'centerElement', 'centerColor', 'playerX', 'playerY'],
    },
    {
      id: 'short-header-controlled-reopen',
      input: Buffer.from([0x03]),
      expectedOutcome: 1,
      expected: {
        ...readPascalSummary('BRDHDR.OUT'),
        name: Buffer.from('BEFORE', 'latin1'),
      },
      evidence: 'DP',
      fields: ['nameLength', 'maxShots', 'statCount', 'centerElement', 'centerColor', 'playerX', 'playerY', 'worldBoardLength'],
    },
  ];

  for (const test of cases) {
    new Uint8Array(memory.buffer, input_ptr(), test.input.length).set(test.input);
    const result = run_editor_brd_transfer(test.input.length);
    if (result !== 0) throw new Error(`${test.id}: shared transfer returned ${result}`);
    const output = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    if (output.length < 4) throw new Error(`${test.id}: missing outcome, board length, or production board state`);
    if (output[0] !== test.expectedOutcome) {
      throw new Error(`${test.id}: expected production outcome ${test.expectedOutcome}, got ${output[0]}`);
    }
    const actual = parseProductionBoard(output.subarray(3));
    actual.worldBoardLength = output.readInt16LE(1);
    compare(`BRDXFER ${test.id}`, test.expected, actual, test.fields);
    if (test.expected.name && !test.expected.name.equals(actual.name)) {
      throw new Error(`${test.id}: board name differs; expected ${test.expected.name.toString('latin1')}, production=${actual.name.toString('latin1')}`);
    }
    if (test.expected.neighbors && test.expected.neighbors.some((value, index) => value !== actual.neighbors[index])) {
      throw new Error(`${test.id}: neighbors differ; expected ${test.expected.neighbors}, production=${actual.neighbors}`);
    }
    if (test.id === 'empty-header-controlled-reopen'
        && actual.worldBoardLength !== test.expected.worldBoardLength + 2) {
      throw new Error(`${test.id}: private LibZoo board-buffer length did not include expected two-byte metadata overhead; Pascal=${test.expected.worldBoardLength}, production=${actual.worldBoardLength}`);
    }
    console.log(`BRDXFER ${test.id}: outcome ${test.expectedOutcome}; ${test.fields.length} selected state fields match (${test.evidence}).`);
  }
})().catch((error) => {
  console.error(error.message);
  process.exitCode = 1;
});
