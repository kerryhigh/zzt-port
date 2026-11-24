const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-pusher-duplicator-tick.js PUSHDUP.OUT oop_movement.wasm');
}

const rows = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
const cases = [
  ...Array.from({ length: 5 }, (_, i) => ['dup-tick', i + 1]),
  ...Array.from({ length: 2 }, (_, i) => ['pusher-tick', i + 1]),
];
const fieldsPerCase = 61;
if (rows.length !== cases.length) {
  throw new Error(`Turbo Pascal Pusher/Duplicator probe should contain ${cases.length} cases; got ${rows.length}`);
}

const pascalValues = rows.map((row, index) => {
  const [label, ...rawFields] = row.trim().split(/\s+/);
  const values = rawFields.map(Number);
  const [expectedLabel, expectedId] = cases[index];
  if (label !== expectedLabel || values.length !== fieldsPerCase
    || values.some((value) => !Number.isInteger(value)) || values[0] !== expectedId) {
    throw new Error(`Malformed or out-of-order Turbo Pascal case ${index + 1}: ${row}`);
  }
  return values;
});

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const exports = instance.exports;

  const duplicateStatus = exports.run_duplicator_tick_capture_checks();
  if (duplicateStatus !== 0) throw new Error(`Production Duplicator tick capture returned ${duplicateStatus}`);
  const duplicatePointer = exports.duplicator_tick_output_ptr();
  const duplicateValues = new Int16Array(exports.memory.buffer, duplicatePointer, 5 * fieldsPerCase);

  const pusherStatus = exports.run_pusher_tick_capture_checks();
  if (pusherStatus !== 0) throw new Error(`Production Pusher tick capture returned ${pusherStatus}`);
  const pusherPointer = exports.pusher_tick_output_ptr();
  const pusherValues = new Int16Array(exports.memory.buffer, pusherPointer, 2 * fieldsPerCase);

  const productionValues = [...duplicateValues, ...pusherValues];
  const fieldNames = [
    'case id', 'stat count',
    ...['stat 1', 'stat 2', 'new stat'].flatMap((stat) => [
      `${stat} x`, `${stat} y`, `${stat} step x`, `${stat} step y`, `${stat} cycle`,
      `${stat} p1`, `${stat} p2`, `${stat} p3`, `${stat} follower`, `${stat} leader`,
      `${stat} under element`, `${stat} under color`,
    ]),
    ...Array.from({ length: 6 }, (_, i) => [`tile ${i + 9} element`, `tile ${i + 9} color`]).flat(),
    'sound priority', 'sound playing', 'sound length',
    ...Array.from({ length: 8 }, (_, i) => `sound byte ${i + 1}`),
  ];

  for (let caseIndex = 0; caseIndex < cases.length; caseIndex++) {
    for (let fieldIndex = 0; fieldIndex < fieldsPerCase; fieldIndex++) {
      const flatIndex = caseIndex * fieldsPerCase + fieldIndex;
      if (productionValues[flatIndex] !== pascalValues[caseIndex][fieldIndex]) {
        throw new Error(`${cases[caseIndex][0]} ${cases[caseIndex][1]} ${fieldNames[fieldIndex]} differs: production ${productionValues[flatIndex]}, Turbo Pascal ${pascalValues[caseIndex][fieldIndex]}`);
      }
    }
  }

  console.log('Turbo Pascal versus production Pusher/Duplicator ticks: cooldown, stat cloning/copy, stat limits, recursive push and blocked-chain snapshots match across seven cases.');
})();
