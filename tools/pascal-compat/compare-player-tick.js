const fs = require('node:fs');
const path = require('node:path');

const [pascalDirectory, wasmPath] = process.argv.slice(2);
if (!pascalDirectory || !wasmPath) {
  throw new Error('Usage: node compare-player-tick.js <Pascal-output-directory> oop_movement.wasm');
}

function readProbe(filename, expectedLabels, expectedFieldCounts) {
  const rows = fs.readFileSync(path.join(pascalDirectory, filename), 'ascii')
    .trim().split(/\r?\n/);
  if (rows.length !== expectedLabels.length) {
    throw new Error(`${filename} should contain ${expectedLabels.length} cases; got ${rows.length}`);
  }
  return rows.map((row, index) => {
    const tokens = row.trim().split(/\s+/);
    if (tokens[0] !== expectedLabels[index]) {
      throw new Error(`${filename} case ${index + 1} should be ${expectedLabels[index]}; got ${tokens[0]}`);
    }
    const values = tokens.slice(1)
      .map((token) => token === 'TRUE' ? '1' : token === 'FALSE' ? '0' : token)
      .filter((token) => /^-?\d+$/.test(token))
      .map(Number);
    if (values.length !== expectedFieldCounts[index]) {
      throw new Error(`${filename} case ${index + 1} has ${values.length} numeric fields; expected ${expectedFieldCounts[index]}`);
    }
    return values;
  });
}

function compareProbe(instance, probe) {
  const expectedRows = readProbe(probe.filename, probe.labels, probe.fieldCounts);
  const output = new Int16Array(instance.exports.memory.buffer,
    instance.exports[probe.pointer](), probe.labels.length * probe.stride);
  for (let caseIndex = 0; caseIndex < expectedRows.length; caseIndex++) {
    const expected = expectedRows[caseIndex];
    let comparedField = 0;
    for (let fieldIndex = 0; fieldIndex < expected.length; fieldIndex++) {
      if (probe.ignoredFields?.includes(fieldIndex)) continue;
      const actual = output[caseIndex * probe.stride + comparedField];
      if (actual !== expected[fieldIndex]) {
        throw new Error(`${probe.filename} ${probe.labels[caseIndex]} field ${fieldIndex + 1} differs: production ${actual}, Turbo Pascal ${expected[fieldIndex]}`);
      }
      comparedField++;
    }
    const expectedComparedFields = expected.length - (probe.ignoredFields?.length ?? 0);
    if (comparedField !== expectedComparedFields || comparedField > probe.stride) {
      throw new Error(`${probe.filename} ${probe.labels[caseIndex]} compared ${comparedField} fields; expected ${expectedComparedFields}`);
    }
  }
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_player_tick_capture_checks();
  if (status !== 0) throw new Error(`Production player-tick capture returned ${status}`);

  const probes = [
    {
      filename: 'PLAYERMV.OUT',
      labels: ['move-empty', 'move-blocked', 'move-push', 'move-push-blocked'],
      fieldCounts: [17, 17, 17, 17],
      stride: 17,
      pointer: 'player_movement_output_ptr'
    },
    {
      filename: 'PLAYERST.OUT',
      labels: ['shoot-held-right', 'shoot-space-down', 'shoot-blocked', 'shoot-limit', 'shoot-space-no-direction'],
      fieldCounts: [24, 24, 10, 10, 10],
      stride: 24,
      pointer: 'player_shoot_output_ptr'
    },
    {
      filename: 'PLAYERKY.OUT',
      labels: ['pause-key', 'sound-key'],
      fieldCounts: [12, 7],
      stride: 12,
      pointer: 'player_key_output_ptr'
    },
    {
      filename: 'PLAYERDE.OUT',
      labels: ['dead-space', 'dead-existing-stat'],
      fieldCounts: [18, 18],
      stride: 18,
      pointer: 'player_death_output_ptr'
    },
    {
      filename: 'PAUSEMV.OUT',
      labels: ['pause-open', 'pause-cycle', 'pause-blocked'],
      fieldCounts: [15, 15, 15],
      ignoredFields: [12],
      stride: 14,
      pointer: 'player_paused_move_output_ptr'
    }
  ];
  for (const probe of probes) compareProbe(instance, probe);
  console.log('Turbo Pascal versus production player ticks: movement, open and blocked paused moves, the following neutral player tick, five shooting cases, pause/sound keys, and both death cases match field-for-field.');
})().catch((error) => { console.error(error); process.exitCode = 1; });
