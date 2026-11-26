#!/usr/bin/env node
'use strict';

const crypto = require('node:crypto');
const fs = require('node:fs');
const { spawnSync } = require('node:child_process');

const [comparatorPath, pascalPath, wasmPath, soundPath, cellPath, savePath,
  loadResultPath, roundtripSavePath, mutatedPath] = process.argv.slice(2);
if ([comparatorPath, pascalPath, wasmPath, soundPath, cellPath, savePath,
  loadResultPath, roundtripSavePath, mutatedPath].some(value => !value)) {
  throw new Error('Usage: check-integrated-diagnostics.js <comparator> <INTEGPR.OUT> <wasm> <sound> <cell> <save> <load-result> <roundtrip-save> <mutated-output>');
}

const source = fs.readFileSync(pascalPath);
const sourceHash = crypto.createHash('sha256').update(source).digest('hex');
const lines = source.toString('ascii').trimEnd().split(/\r?\n/);
const cycleOneIndex = lines.findIndex(line => {
  const fields = line.trim().split(/\s+/);
  return fields[0] === 'C' && fields[1] === '1';
});
if (cycleOneIndex < 0) throw new Error('Pascal replay output has no C1 cycle checkpoint');

const cycleOne = lines[cycleOneIndex].trim().split(/\s+/);
if (cycleOne.length < 4 || !Number.isInteger(Number(cycleOne[3]))) {
  throw new Error('Pascal C1 checkpoint has no first integer state field');
}
cycleOne[3] = String(Number(cycleOne[3]) + 1);
lines[cycleOneIndex] = cycleOne.join(' ');
fs.writeFileSync(mutatedPath, `${lines.join('\n')}\n`, 'ascii');

const result = spawnSync(process.execPath, [comparatorPath, mutatedPath, wasmPath,
  soundPath, cellPath, savePath, loadResultPath, roundtripSavePath], {
  encoding: 'utf8',
  windowsHide: true,
});
if (result.error) throw result.error;
const diagnostic = `${result.stdout || ''}${result.stderr || ''}`;
const expected = /First divergence at cycle 1, field 0 \(ammo\): Pascal=-?\d+, production=-?\d+;.*context=\[ammo:/;
if (result.status !== 1 || !expected.test(diagnostic)) {
  throw new Error(`Integrated first-divergence negative control failed (exit ${result.status}): ${diagnostic.trim()}`);
}
const sourceHashAfter = crypto.createHash('sha256').update(fs.readFileSync(pascalPath)).digest('hex');
if (sourceHashAfter !== sourceHash) throw new Error('Diagnostic check modified the original Pascal oracle output');

const firstDivergenceLine = diagnostic.split(/\r?\n/)
  .find(line => line.includes('First divergence at'))
  ?.replace(/^Error:\s*/, '');
if (!firstDivergenceLine) throw new Error('Comparator exited without a first-divergence report line');
console.log(`Integrated diagnostic negative control passed: ${firstDivergenceLine}; original Pascal output SHA-256 unchanged (${sourceHash}).`);
