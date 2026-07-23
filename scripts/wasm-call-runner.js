// Calls a single exported, no-argument, i32-returning function from a
// compiled wasm module and exits with that value as the process exit
// code -- the wasm-appropriate counterpart of scripts/qemu-runner
// piping a real binary's stdout into `cmp`: ec has no printf/WASI
// runtime wired up yet (see docs/notes_wasm.txt), so a self-checking
// function returning 0-for-pass/nonzero-for-fail (see
// tests/c/mini2/regress_wasm.c) is what stands in for comparing
// stdout against an expected.txt until it does.
//
// Usage: node wasm-call-runner.js FILE.wasm FUNCNAME

const fs = require('fs');

const file = process.argv[2];
const func = process.argv[3];
if (!file || !func) {
  console.error('usage: wasm-call-runner.js FILE.wasm FUNCNAME');
  process.exit(255);
}

const bytes = fs.readFileSync(file);
WebAssembly.compile(bytes).then((module) => {
  const instance = new WebAssembly.Instance(module, {});
  if (typeof instance.exports[func] !== 'function') {
    console.error(`wasm-call-runner: no exported function '${func}'`);
    process.exit(255);
  }
  const result = instance.exports[func]();
  process.exit(result);
}).catch((e) => {
  console.error('wasm-call-runner: ' + e);
  process.exit(255);
});
