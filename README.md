# goken9cc

**goken9cc** is a portable, multi-architecture toolchain &mdash; C compilers,
assemblers, and linkers &mdash; together with a minimalist C library, rooted in
Ken Thompson's legendary work on the Plan 9 and Inferno operating
systems. First extended by Go developers, this toolchain now brings
cross-platform support for Linux, macOS, and Windows while preserving
the simplicity, elegance, and efficiency of the original Plan 9 tools.

See https://www.youtube.com/watch?v=E3iUpyqKvgk for a presentation of the project.

---

## News

- **Q3 2026** &mdash; v0.4 (in progress): a big unification release.
  - imported the arm and x86 tools (5a/5c/5l, 8a/8c/8l) as rewritten ("Lpized") for the Principia Softwarica books back into goken9cc, fixing the many mismatches this uncovered against the older kencc-derived variants (kept side by side as 5ak/8ak, etc.)
  - native macOS (Mach-O) output for 6l/7l
  - native Windows (PE) output for 6l/8l
  - early WebAssembly backend (ea/ec/el)
  - separately ported a large batch of 9front fixes into the mips and arm64 toolchains (va/vc/vl, 7a/7c/7l)
  - brought up riscv64 for real (it was only commented-out stubs before)
  - fixed the arm and mips emulators (5i/vi) so they actually run their hello-world tests
  - much richer test infra: tests now run under qemu (Linux), wine (Windows), and Node (wasm), and check output against expected.txt instead of just checking the build succeeds; new tests/s/variants and tests/c/variants compare the object files and executables produced by the principia vs. kencc lineages to catch mismatches
- **Q2 2026** &mdash; v0.3: presented goken9cc at IWP9, the International Workshop on Plan 9.
- **Q1 2026** &mdash; v0.2: added `pcc`, "a portable C compiler" to compile legacy Unix programs (called APE in Plan 9); started using AddressSanitizer to catch memory bugs in the toolchain itself; wrote up the project for an IWP9 paper submission.
- **Q4 2025** &mdash; v0.1: first working release &mdash; a Plan 9-style toolchain (compilers, assemblers, linkers for arm, x86, mips, and early riscv, plus an arm/mips emulator and the acid debugger) able to cross compile Principia Softwarica's own `pc`/`pi` operating system targets.
- **Q3 2025** &mdash; Beta: arm64 and arm32 Linux binaries actually working; imported `mk`, `rc`, `ed`, and other core utilities from Principia Softwarica so the repo is self-contained.
- **Q2 2025** &mdash; Alpha: project started &mdash; forked the Go repository at its October 2010 C-toolchain commit, refocused on the C toolchain (dropping Go itself), first (untested) imports of the mips, arm64, and riscv toolchains, first Docker/Nix/CI setup.

goken9cc's direct ancestor is [fork-kencc](https://github.com/aryx/fork-kencc), now deprecated in its favor:

- **Q2 2025** &mdash; fork-kencc v0.4: added a Dockerfile and CI.
- **Q2 2024** &mdash; fork-kencc v0.3: small resume of the project after a hiatus, with basic CI.
- **2019** &mdash; **2024**: Hiatus (working on https://semgrep.dev)

- **Q2 2018** &mdash; fork-kencc v0.2: got it building also on Linux and Windows.
- **Q1 2014** &mdash; fork-kencc v0.1: project started &mdash; forked the kencc toolchain from Plan 9, keep just the x86 and arm toolchains, got it building on macOS, and did a big code reorganization.

See [changes.txt](changes.txt) for the detailed changelog.

---

## Features

- **Portable:**
  It can *build* on Linux, macOS, and Windows (WSL, Cygwin) (TODO Plan 9 and xv6) using gcc or clang (TODO or a boostrapped version of itself), on 32 bits or 64 bits machines
- **Multi-OS support:** 
  Link C and assembly programs that can *run* on Linux, macOS, Windows, and Plan 9 (TODO xv6)
- **Multi-architecture support:**
  Build C and assembly programs *targeting* the 386 (a.k.a. x86), amd64 (a.k.a. x86_64), arm, arm64 (a.k.a. aarch64), riscv (a.k.a. riscv32), riscv64, and mips architectures, plus experimental WebAssembly (wasm) support
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
| 6 | amd64 | 6c | 6a | 6l | .6 |
| 7 | arm64 | 7c | 7a | 7l | .7 |
| 8 | x86 (386) | 8c | 8a | 8l | .8 |
| e | wasm | ec | ea | el | .e |
| i | riscv | ic | ia | il | .i |
| j | riscv64 | jc | ja | jl | .j |
| v | mips | vc | va | vl | .v |

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

This fork was then further extended to add more architectures such as
arm64 (thanks to the work of Charles Forsyth) and
riscv (thanks to the work of Richard Miller) that
were not in the Go repo but scattered around in "kencc-derived" repositories.

More work was done then to make all of this work together,
to support the latest Linux, macOS, and Windows to reach
a point where one could use goken9cc to compile goken9cc itself
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
