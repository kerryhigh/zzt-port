const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-flags.js OOPFLAGS.OUT oop_label.wasm');
}

const maxFlags = 10;
const payloadLength = 20;
const mapBytes = maxFlags * (payloadLength + 1);
const actualBytes = 2 * mapBytes + 7 * 2;

function parsePascalRows() {
  const lines = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
  if (lines.length !== 22) {
    throw new Error(`Expected 22 Pascal OOP flag rows, found ${lines.length}`);
  }
  let cursor = 0;
  const readMap = (prefix) => {
    const rows = [];
    for (let slot = 1; slot <= maxFlags; slot++) {
      const fields = lines[cursor++].trim().split(/\s+/);
      if (fields.length !== payloadLength + 3
        || fields[0] !== prefix
        || Number(fields[1]) !== slot) {
        throw new Error(`Malformed Pascal flag row ${prefix}${slot}: ${fields.join(' ')}`);
      }
      const values = fields.slice(2).map(Number);
      if (values.some((value) => !Number.isInteger(value) || value < 0 || value > 255)) {
        throw new Error(`Invalid Pascal flag bytes in row ${prefix}${slot}`);
      }
      rows.push(values);
    }
    return rows;
  };
  const readIds = (prefix, count) => {
    const fields = lines[cursor++].trim().split(/\s+/);
    if (fields.length !== count + 1 || fields[0] !== prefix) {
      throw new Error(`Malformed Pascal ${prefix} flag lookup row: ${fields.join(' ')}`);
    }
    return fields.slice(1).map(Number);
  };
  const first = readMap('A');
  const firstIds = readIds('I', 4);
  const full = readMap('F');
  const fullIds = readIds('J', 3);
  return { first, firstIds, full, fullIds };
}

function expectedMap(names, markers) {
  return names.map((name, index) => {
    const bytes = Uint8Array.from(Array(payloadLength).fill(markers[index]));
    const encoded = Buffer.from(name, 'ascii');
    encoded.copy(Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength));
    return [encoded.length, ...bytes];
  });
}

function compareMap(label, actual, expected) {
  for (let slot = 0; slot < maxFlags; slot++) {
    for (let byte = 0; byte <= payloadLength; byte++) {
      if (actual[slot][byte] !== expected[slot][byte]) {
        throw new Error(`${label} Pascal flag slot ${slot + 1}, byte ${byte} differs: ${actual[slot][byte]} != ${expected[slot][byte]}`);
      }
    }
  }
}

function readSigned16(bytes, offset) {
  const value = bytes[offset] | (bytes[offset + 1] << 8);
  return value & 0x8000 ? value - 0x10000 : value;
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const exports = instance.exports;
  const result = exports.run_oop_flag_state_capture_checks();
  if (result !== 0) throw new Error(`Production OOP flag capture returned ${result}`);
  const pointer = exports.oop_flag_capture_output_ptr();
  const actual = new Uint8Array(exports.memory.buffer, pointer, actualBytes);
  const pascal = parsePascalRows();
  const firstExpected = expectedMap(
    ['ALPHA', 'BETA', '', 'TAIL', '', '', '', '', '', ''],
    Array.from({ length: maxFlags }, (_, index) => 0xA1 + index)
  );
  const firstPreviousDuplicate = Uint8Array.from(Array(payloadLength).fill(0xA3));
  Buffer.from('DUP', 'ascii').copy(Buffer.from(firstPreviousDuplicate.buffer));
  firstExpected[2] = [0, ...firstPreviousDuplicate];
  const fullExpected = expectedMap(
    ['F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8', 'F9', 'OVERFLOW'],
    Array.from({ length: maxFlags }, (_, index) => 0xB1 + index)
  );
  compareMap('duplicate-clear/reuse', pascal.first, firstExpected);
  compareMap('full-table overwrite', pascal.full, fullExpected);

  const firstMap = actual.subarray(0, mapBytes);
  const firstActualRows = Array.from({ length: maxFlags }, (_, slot) =>
    Array.from(firstMap.subarray(slot * (payloadLength + 1), (slot + 1) * (payloadLength + 1)))
  );
  const fullMapOffset = mapBytes + 4 * 2;
  const fullMap = actual.subarray(fullMapOffset, fullMapOffset + mapBytes);
  const fullActualRows = Array.from({ length: maxFlags }, (_, slot) =>
    Array.from(fullMap.subarray(slot * (payloadLength + 1), (slot + 1) * (payloadLength + 1)))
  );
  compareMap('duplicate-clear/reuse production', firstActualRows, firstExpected);
  compareMap('full-table overwrite production', fullActualRows, fullExpected);

  const firstIds = [1, -1, 2, -1];
  const firstIdOffset = mapBytes;
  for (let index = 0; index < firstIds.length; index++) {
    const pascalId = pascal.firstIds[index];
    const productionId = readSigned16(actual, firstIdOffset + index * 2);
    if (pascalId !== firstIds[index] || productionId !== (pascalId < 0 ? -1 : pascalId - 1)) {
      throw new Error(`Flag lookup ${index} differs: Pascal ${pascalId}, production ${productionId}`);
    }
  }
  const fullIds = [1, -1, 10];
  const fullIdOffset = fullMapOffset + mapBytes;
  for (let index = 0; index < fullIds.length; index++) {
    const pascalId = pascal.fullIds[index];
    const productionId = readSigned16(actual, fullIdOffset + index * 2);
    if (pascalId !== fullIds[index] || productionId !== (pascalId < 0 ? -1 : pascalId - 1)) {
      throw new Error(`Full-table flag lookup ${index} differs: Pascal ${pascalId}, production ${productionId}`);
    }
  }

  for (let slot = 0; slot < maxFlags; slot++) {
    for (let byte = 0; byte <= payloadLength; byte++) {
      if (pascal.first[slot][byte] !== firstActualRows[slot][byte]) {
        throw new Error(`Pascal and production flag bytes differ in first map slot ${slot + 1}, byte ${byte}`);
      }
      if (pascal.full[slot][byte] !== fullActualRows[slot][byte]) {
        throw new Error(`Pascal and production flag bytes differ in full map slot ${slot + 1}, byte ${byte}`);
      }
    }
  }

  console.log('Turbo Pascal versus production OOP flags: last-duplicate #CLEAR, first empty-slot #SET, preserved short-string tail bytes, duplicate suppression, and full-table last-slot overwrite match across all ten slots.');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
