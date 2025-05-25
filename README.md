# goken9cc

**goken9cc** is a C compiler, assembler, and linker toolchain
rooted in the legendary work of Ken Thompson and the Plan 9 and
Inferno operating systems. Originally extended by Go developers,
this toolchain brings cross-platform support for Linux, macOS, and
Windows while preserving the simplicity, elegance, and efficiency of
the original Plan 9 tools.

---

## Features

- **A portable compiler:**
  It can build on Linux, macOS, and Windows (long term goal is buildable on Plan 9 too)
- **Cross-OS support:** 
  Build C programs running on Linux, macOS, Windows, and Plan 9
- **Cross-architecture support:**
  Build C programs targeting the 386 (a.k.a., x86), amd64 (a.k.a. x86_64), and arm
  architectures (long-term goal is to support also arm64, RISC V, and Wasm)
- **Compact and efficient:**
  A lightweight compiler toolchain designed for speed and simplicity.
- **Heritage:**
  Based on the Plan 9 and Inferno OS compilers developed by Ken Thompson and others.
- **Go-era improvements:**
  Enhanced and extended by contributors from the Go language community to support
  modern development needs.
- **Open and extensible:**
  Designed to be easy to understand, modify, and integrate into new projects.

---

## Getting Started

### Building from source

```bash
git clone https://github.com/aryx/goken9cc.git
cd goken9cc
configure
make
make install
```
