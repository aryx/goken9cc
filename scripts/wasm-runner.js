// Run a .wasm module produced by linkers/el under Node's WASI
// implementation, the wasm counterpart of scripts/qemu-runner (see
// that file's comment for why tests shell out to a small runner
// instead of reimplementing "how do I execute this" inline).
//
// v1 only supports el's own hardcoded import shape (see linkers/el/
// l.h's Import comment): every import is bound against the
// wasi_snapshot_preview1 namespace. Usage: node wasm-runner.js FILE.wasm

const { WASI } = require('node:wasi');
const fs = require('fs');

const file = process.argv[2];
if (!file) {
  console.error('usage: wasm-runner.js FILE.wasm');
  process.exit(1);
}

const wasi = new WASI({ version: 'preview1', args: [], env: {} });
const bytes = fs.readFileSync(file);

WebAssembly.compile(bytes).then((module) => {
  const instance = new WebAssembly.Instance(module, {
    wasi_snapshot_preview1: wasi.wasiImport,
  });
  wasi.start(instance);
}).catch((e) => {
  console.error('wasm-runner: ' + e);
  process.exit(1);
});
