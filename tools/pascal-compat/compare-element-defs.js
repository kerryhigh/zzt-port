#!/usr/bin/env node
const fs = require('node:fs');

const [pascalPath, cTablePath] = process.argv.slice(2);
if (!pascalPath || !cTablePath) {
  console.error('Usage: node compare-element-defs.js <ELDEFPRB.OUT> <zoo_element_defs.inc>');
  process.exit(2);
}

const pascalRows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/).map((line, index) => {
  const values = line.trim().split(/\s+/);
  if (values.length !== 22 || values.some((value) => value.length === 0)) {
    throw new Error(`Pascal element definition row ${index} has ${values.length} fields; expected 22.`);
  }
  const numeric = values.slice(0, 14).map(Number);
  if (numeric.some((value) => !Number.isInteger(value))) {
    throw new Error(`Pascal element definition row ${index} has a non-integer field.`);
  }
  return {
    id: numeric[0],
    character: numeric[1],
    color: numeric[2],
    destructible: numeric[3],
    pushable: numeric[4],
    visibleInDark: numeric[5],
    placeableOnTop: numeric[6],
    walkable: numeric[7],
    hasDrawProc: numeric[8],
    cycle: numeric[9],
    editorCategory: numeric[10],
    editorShortcut: numeric[11],
    scoreValue: numeric[12],
    hasText: numeric[13],
    strings: values.slice(14),
  };
});

const source = fs.readFileSync(cTablePath, 'utf8');
const cRows = [];
let current = null;
for (const line of source.split(/\r?\n/)) {
  const start = line.match(/^\s*\{\s*\/\/\s*(\d+)(?:\s*-.*)?$/);
  if (start) {
    if (current) throw new Error('Nested element table row while reading production table.');
    current = { id: Number(start[1]), fields: Object.create(null) };
    cRows.push(current);
    continue;
  }
  if (!current) continue;
  const field = line.match(/^\s*(.+?),\s*\/\/\s*(.+?)\s*$/);
  if (field) current.fields[field[2]] = field[1].trim();
  if (/^\s*\},?\s*$/.test(line)) current = null;
}
if (current) throw new Error('Production element table ended before its final row closed.');

function integer(value, field) {
  if (/^true$/.test(value)) return 1;
  if (/^false$/.test(value)) return 0;
  const macros = {
    ZOO_COLOR_CHOICE_ON_BLACK: 0xff,
    ZOO_COLOR_WHITE_ON_CHOICE: 0xfe,
    ZOO_COLOR_CHOICE_ON_CHOICE: 0xfd,
    ZOO_CATEGORY_ITEM: 1,
    ZOO_CATEGORY_CREATURE: 2,
    ZOO_CATEGORY_TERRAIN: 3,
  };
  if (Object.hasOwn(macros, value)) return macros[value];
  if (/^0x[\da-f]+$/i.test(value)) return Number.parseInt(value.slice(2), 16);
  if (/^-?\d+$/.test(value)) return Number(value);
  const character = value.match(/^'(.*)'$/);
  if (character) {
    const escapes = { '\\0': 0, '\\n': 10, '\\r': 13, '\\t': 9, '\\\\': 92, "\\'": 39 };
    const inner = character[1];
    if (Object.hasOwn(escapes, inner)) return escapes[inner];
    if (inner.length === 1) return inner.charCodeAt(0);
  }
  throw new Error(`Unknown production value for ${field}: ${value}`);
}

function cString(value, field) {
  if (!/^".*"$/.test(value)) throw new Error(`Expected C string for ${field}, got ${value}`);
  return Buffer.from(value.slice(1, -1), 'ascii').toString('hex').toUpperCase();
}

const stringFields = [
  'name', 'category name', 'param 1 name', 'param 2 name',
  'param bullet type name', 'param board name', 'param dir name', 'param text name',
];
const scalarFields = [
  ['character', 'character'], ['color', 'color'], ['destructible', 'destructible'],
  ['pushable', 'pushable'], ['visible in dark', 'visibleInDark'],
  ['placeable on top', 'placeableOnTop'], ['walkable', 'walkable'],
  ['has draw func', 'hasDrawProc'], ['cycle', 'cycle'],
  ['editor category', 'editorCategory'], ['editor shortcut', 'editorShortcut'],
  ['score value', 'scoreValue'], ['has text (libzoo)', 'hasText'],
];

if (pascalRows.length !== 54 || cRows.length !== 54) {
  throw new Error(`Expected 54 definitions in each table, found Pascal=${pascalRows.length}, production=${cRows.length}.`);
}

const differences = [];
for (let id = 0; id < 54; id++) {
  const pascal = pascalRows[id];
  const c = cRows[id];
  if (pascal.id !== id || c.id !== id) throw new Error(`Element table index mismatch at ${id}.`);
  const actual = Object.create(null);
  for (const [cField, key] of scalarFields) actual[key] = integer(c.fields[cField], `${id}.${cField}`);
  for (let i = 0; i < stringFields.length; i++) {
    actual[`string${i}`] = cString(c.fields[stringFields[i]], `${id}.${stringFields[i]}`);
  }
  const expected = {
    character: pascal.character, color: pascal.color, destructible: pascal.destructible,
    pushable: pascal.pushable, visibleInDark: pascal.visibleInDark,
    placeableOnTop: pascal.placeableOnTop, walkable: pascal.walkable,
    hasDrawProc: pascal.hasDrawProc, cycle: pascal.cycle,
    editorCategory: pascal.editorCategory, editorShortcut: pascal.editorShortcut,
    scoreValue: pascal.scoreValue, hasText: pascal.hasText,
  };
  for (let i = 0; i < pascal.strings.length; i++) {
    expected[`string${i}`] = pascal.strings[i] === '-' ? '' : pascal.strings[i].toUpperCase();
  }
  for (const [key, expectedValue] of Object.entries(expected)) {
    if (actual[key] === expectedValue) continue;
    // libzoo uses a draw callback for the player to animate its character in
    // place; the Pascal source instead mutates ElementDefs[4].Character.
    if (id === 4 && key === 'hasDrawProc' && expectedValue === 0 && actual[key] === 1) continue;
    differences.push(`element ${id} ${key}: Pascal=${JSON.stringify(expectedValue)} production=${JSON.stringify(actual[key])}`);
  }
}

if (differences.length) {
  console.error(`Element definition comparison failed (${differences.length} differences):`);
  for (const difference of differences) console.error(`  ${difference}`);
  process.exit(1);
}
console.log('Element definition comparison passed: all 54 Pascal/production metadata rows match (Player draw callback is the documented adapter difference).');
