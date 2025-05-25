# goken9cc

**goken9cc** is a portable multi-platform C compiler, assembler, and
linker toolchain as well as a minimalist C library rooted in the
legendary work of Ken Thompson and the Plan 9 and Inferno operating
systems. Originally extended by Go developers, this toolchain brings
cross-platform support for Linux, macOS, and Windows while preserving
the simplicity, elegance, and efficiency of the original Plan 9 tools.

---

## Features

- **Portable:**
  It can *build* on Linux, macOS, and Windows (TODO and Plan 9) using gcc or clang
  (TODO or a boostrapped version of itself)
- **Multi-OS support:** 
  Build C programs that can *run* on Linux, macOS, Windows, and Plan 9
- **Multi-architecture support:**
  Build C programs *targeting* the 386 (a.k.a. x86), amd64 (a.k.a. x86_64), and arm
  architectures (TODO arm64, RISC V, and Wasm)
- **Cross-compilers:**
  Build C programs targeting different platforms from different platforms
  (e.g., you can build from a Linux 386 machine a binary for amd64 macOS)
- **Compact and efficient:**
  A lightweight compiler toolchain designed for speed and simplicity.
- **Heritage:**
  Based on the Plan 9 and Inferno OS compilers developed by Ken Thompson and others.
- **Go-era improvements:**
  Enhanced and extended by contributors from the Go language community for
  multi-platform support.
- **Open and extensible:**
  Designed to be easy to understand, modify, and integrate into new projects
  thanks to its reasonable size and the use of *Literate programming* 
  explaining in depth the code.

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

You can also play with the `$GOOS` and `$GOARCH` environment variables
for cross compiling.
