const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-any.js OOPANY.OUT oop_label.wasm');
}

const lines = fs.readFileSync(pascalPath, 'ascii').trim().split(/\r?\n/);
if (lines.length !== 54) {
  throw new Error(`Turbo Pascal ANY probe should contain one result for each element ID 0 through 53; got ${lines.length}`);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const inputPointer = instance.exports.oop_any_name_ptr();
  const outputPointer = instance.exports.oop_any_output_ptr();
  const input = new Uint8Array(instance.exports.memory.buffer, inputPointer, 64);
  let namedCount = 0;

  for (let elementId = 0; elementId <= 53; elementId++) {
    const fields = lines[elementId].trim().split(/\s+/);
    if (fields[0] !== String(elementId)) {
      throw new Error(`Turbo Pascal ANY output is out of order at element ${elementId}: ${lines[elementId]}`);
    }
    if (fields.length === 2 && fields[1] === 'SKIP') continue;
    if (fields.length !== 4 || !/^[A-Z0-9]{1,50}$/.test(fields[1])
      || (fields[2] !== '0' && fields[2] !== '1')
      || (fields[3] !== '0' && fields[3] !== '1')) {
      throw new Error(`Malformed Turbo Pascal ANY output for element ${elementId}: ${lines[elementId]}`);
    }
    if (fields[2] !== '1' || fields[3] !== '0') {
      throw new Error(`Turbo Pascal ANY name ${fields[1]} did not match only its present tile: ${lines[elementId]}`);
    }

    input.fill(0);
    input.set(Buffer.from(fields[1], 'ascii'));
    const status = instance.exports.run_oop_any_element_check(elementId);
    if (status !== 0) throw new Error(`Production ANY parser rejected element ${elementId} (${fields[1]}): ${status}`);
    const actual = new Uint8Array(instance.exports.memory.buffer, outputPointer + elementId * 2, 2);
    if (actual[0] !== Number(fields[2]) || actual[1] !== Number(fields[3])) {
      throw new Error(`ANY element ${elementId} (${fields[1]}) differs: production ${actual[0]}/${actual[1]}, Turbo Pascal ${fields[2]}/${fields[3]}`);
    }
    namedCount++;
  }

  if (namedCount !== 42) throw new Error(`Expected 42 named Pascal tile elements, compared ${namedCount}`);
  console.log('Turbo Pascal versus production OOP #IF ANY: all 42 named tile elements match when present and miss when absent.');
})();
