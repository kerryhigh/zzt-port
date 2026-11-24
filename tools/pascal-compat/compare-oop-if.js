const fs = require('node:fs');

const [pascalPath, wasmPath] = process.argv.slice(2);
if (!pascalPath || !wasmPath) {
  throw new Error('Usage: node compare-oop-if.js OOPIF.OUT oop_label.wasm');
}

const values = fs.readFileSync(pascalPath, 'ascii').trim().split(/\s+/).map(Number);
if (values.length !== 110 || values.some((value) => !Number.isInteger(value))) {
	throw new Error(`Turbo Pascal IF probe should contain 55 case/result pairs; got ${values.length} values`);
}
const pascalResults = new Uint8Array(55);
for (let i = 0; i < 55; i++) {
  if (values[i * 2] !== i + 1 || (values[i * 2 + 1] !== 0 && values[i * 2 + 1] !== 1)) {
    throw new Error(`Malformed Turbo Pascal IF probe pair at case ${i + 1}`);
  }
  pascalResults[i] = values[i * 2 + 1];
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(wasmPath));
  const instance = await WebAssembly.instantiate(module, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const status = instance.exports.run_oop_if_condition_checks();
  if (status !== 0) throw new Error(`Production OOP IF harness returned ${status}`);
  const pointer = instance.exports.oop_if_output_ptr();
	const productionResults = new Uint8Array(instance.exports.memory.buffer, pointer, 55);
	for (let i = 0; i < 55; i++) {
    if (productionResults[i] !== pascalResults[i]) {
      throw new Error(`OOP IF case ${i + 1} differs: production ${productionResults[i]}, Turbo Pascal ${pascalResults[i]}`);
    }
  }
	console.log('Turbo Pascal versus production OOP #IF: all 55 outcomes match, including recursive NOT, all four board-edge checks, and ANY search bounds.');
})();
