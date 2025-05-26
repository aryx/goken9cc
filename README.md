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
./configure
make
make install
```

You can also play with the `$GOOS` and `$GOARCH` environment variables
for cross compiling.

---

## History

This is a fork of the assemblers, linkers, and C compilers toolchain
as well as supporting libraries (e.g., lib9, libmach, pkg/runtime)
found in the Go source around October 2010 at this precise commit:
https://github.com/golang/go/commit/99a10eff16b79cfb8ccf36e586532a40b17a203c
(see pad.txt for explanation of why I forked at this precise commit).

The Go compiler code was actually itself a fork of the "kencc" toolchain in
inferno-os at https://github.com/inferno-os/inferno-os (in the utils/ subdirectory)
as well as code from the plan9port https://github.com/9fans/plan9port
which both were themselves forks of the kencc toolchain in the plan9 operating
system at https://github.com/plan9foundation/plan9

The main improvements in the Go repository to the C toolchain compared
to the original kencc toolchain in Plan 9 are the support for other
operating systems such as Linux, macOS, and Windows (and not just
Plan9), with the management of binary formats such as ELF (for Linux),
machO (for macOS), and PE (for Windows) as well as the management of
syscalls to those different operating systems. Another nice improvement
was the support for the DWARF debugging format so the generated binaries
could be debugged using gdb (instead of just the Plan9 acid debugger).
