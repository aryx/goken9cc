# goken9cc

**goken9cc** is a portable multi-platform C compiler, assembler, and
linker toolchain as well as a minimalist C library rooted in the
legendary work of Ken Thompson and the Plan 9 and Inferno operating
systems. Originally extended by Go developers, this toolchain brings
cross-platform support for Linux, macOS, and Windows while preserving
the simplicity, elegance, and efficiency of the original Plan 9 tools.

See https://www.youtube.com/watch?v=E3iUpyqKvgk for a presentation of the project.

---

## Features

- **Portable:**
  It can *build* on Linux, macOS, and Windows (WSL, Cygwin) (TODO Plan 9 and xv6) using gcc or clang (TODO or a boostrapped version of itself)
- **Multi-OS support:** 
  Link C and assembly programs that can *run* on Linux, macOS, Windows, and Plan 9 (TODO xv6)
- **Multi-architecture support:**
  Build C and assembly programs *targeting* the 386 (a.k.a. x86), amd64 (a.k.a. x86_64), arm,
  arm64 (a.k.a. aarch64), riscv (a.k.a. riscv32), riscv64, and mips architectures (TODO Wasm)
- **Cross-compilers:**
  Build C programs targeting different platforms from different platforms
  (e.g., you can build from a Linux 386 machine a binary for arm64 macOS)
- **Compact and efficient:**
  A lightweight compiler toolchain designed for speed and simplicity.
- **Heritage:**
  Based on the Plan 9 and Inferno OS compilers developed by Ken Thompson and others.
- **Go-era improvements:**
  Enhanced and extended by contributors from the Go language community for
  multi-platform support.
- **Open and extensible:**
  Designed to be easy to understand, modify, and integrate into new projects,
  thanks to its reasonable size and the use of *Literate programming*, which
  explains the code in depth (see https://principia-softwarica.org).
  It takes more than a license to make code truly open.

---

## Getting Started

### Building from source

```bash
git clone https://github.com/aryx/goken9cc.git
cd goken9cc
./configure
./scripts/build-mk.sh
./scripts/promote-mk.sh
. env.sh
mk
mk install
```

You can also play with the `$GOOS` and `$GOARCH` environment variables
for cross compiling.

---

## Architecture Naming Convention

Plan 9 (and goken9cc) uses single-character codes for architectures. Each tool is prefixed with this code:

| Code | Arch | Compiler | Assembler | Linker | Object ext |
|------|------|----------|-----------|--------|------------|
| 5 | arm | 5c | 5a | 5l | .5 |
| 7 | arm64 | 7c | 7a | 7l | .7 |
| 8 | x86 (386) | 8c | 8a | 8l | .8 |
| 6 | amd64 | 6c | 6a | 6l | .6 |
| v | mips | vc | va | vl | .v |
| i | riscv | ic | ia | il | .i |

Pipeline: `.c` → compiler (`Xc`) → assembler (`Xa`) → linker (`Xl`) → `X.out`

---

## History

This repository was originally a fork of the assemblers, linkers, and C compilers
as well as supporting libraries (e.g., lib9, libmach, pkg/runtime)
in the Go repository in October 2010 at this precise commit:
https://github.com/golang/go/commit/99a10eff16b79cfb8ccf36e586532a40b17a203c
(see pad.org for an explanation of why I forked at this precise commit).

The C toolchain that was part of the Go repository was actually itself
a fork of the "kencc" toolchain in inferno-os at
https://github.com/inferno-os/inferno-os (in the utils/ subdirectory)
as well as code from the plan9port https://github.com/9fans/plan9port
which both were themselves forks of the kencc toolchain in the plan9
operating system at https://github.com/plan9foundation/plan9

The main improvements in the Go repository to the C toolchain compared
to the original kencc toolchain in Plan 9 were the support for other
operating systems such as Linux, macOS, and Windows (and not just
Plan9), with the management of binary formats such as ELF (for Linux),
machO (for macOS), and PE (for Windows) as well as the management of
syscalls to those different operating systems. Another nice improvement
was the support for the DWARF debugging format so the generated binaries
could be debugged using gdb (instead of just the Plan9 acid debugger).

This fork was then further extended to add more architectures
such as arm64 (thanks to the work of Charles Forsyth) and riscv (thanks
to the work of Richard Miller) that were not in the Go repo but
scattered around in "kencc-derived" repositories.

More work was done then to make all of this work together,
to support the latest Linux, macOS, and Windows to reach
a point where one could use goken9cc to compile goken9cc
on many architectures and many operating systems.

---

## Environment variables

build time:
GOARCH
GOOS
GODYNLINK
GOLANG

run time:
for all: GOROOT (used for "#9/..." paths)
for mk: MKSHELL
for rc: RCMAIN
for yacc: YACCPAR
