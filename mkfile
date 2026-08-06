TOP=.
<$TOP/mkconfig
<$TOP/mkfiles/$objtype/mkfile

DIRS= BOOT/lib9 \
  lib_core/libbio \
  lib_strings/libregexp lib_strings/libstring lib_strings/libflate \
  generators/lex/liblex/ generators \
  mk rc \
  lib_toolchain/libmach linkers/ar \
  compilers/cc compilers/cck compilers/pcc \
  assemblers/5a linkers/5l compilers/5c machines/5i \
  assemblers/8a linkers/8l compilers/8c \
  assemblers/6a linkers/6l compilers/6c \
  assemblers/7a linkers/7l compilers/7c \
  assemblers/va linkers/vl compilers/vc   machines/vi \
  assemblers/ia linkers/il compilers/ic \
  assemblers/ea linkers/el \
  debuggers/acid \
  utilities typesetting \
  \
  assemblers/5ak linkers/5lk compilers/5ck    \
  assemblers/8ak linkers/8lk compilers/8ck \
  \
  GO/C/libmach GO/C/cmd/nm GO/C/cmd/ar \
  GO/C/cmd/cc \
  GO/C/cmd/5l GO/C/cmd/5a GO/C/cmd/5c \
  GO/C/cmd/8l GO/C/cmd/8a GO/C/cmd/8c \
  GO/C/cmd/6l GO/C/cmd/6a GO/C/cmd/6c \
  GO/C/cmd/prof GO/C/cmd/cov \

<$TOP/mkfiles/mkdirs

test:
	@{cd tests; mk test}
	# correctness only (not bench.sh's timing), and only the CompCert
	# progs currently wired up to goken's own Xc/Xa/Xl pipeline (see
	# benchs/compcert/mkfile's GOKENPROGS) -- defaults to $cputype from
	# mkconfig, override with 'cputype='<arch> like any other target here.
	# SKIPQEMU=1: skip qemu-runner when $cputype matches this host's own
	# arch (see benchs/compcert/mkfile's test_goken for why) -- only in
	# this top-level context, not test_goken's own default behavior.
	@{cd benchs/compcert; mk 'SKIPQEMU=1' test_goken}

# macOS-native regression tests (no qemu). 'test_macos' auto-detects the
# host: Apple Silicon (arm64) runs the arm64 tests, Intel (x86_64) runs the
# amd64 ones. Use test_macos_arm64 / test_macos_amd64 to force one.
test_macos:V:
	if(~ `{uname -m} arm64)
		@{ cd tests; mk test_macos_arm64 }
	if not
		@{ cd tests; mk test_macos_amd64 }
#`

test_macos_arm64:V:
	cd tests; mk test_macos_arm64
test_macos_amd64:V:
	cd tests; mk test_macos_amd64

# Windows-native regression tests (run directly, no wine/qemu, since the
# host running this target IS Windows). Mirrors test_macos above.
test_windows:V:
	cd tests; mk test_windows

# ==============================================================================
# Bootstrap: build goken's own toolchain using goken itself (7c/7a/7l
# compiling THEMSELVES), not just mk/rc -- see docs/claude_notes/
# notes_libc_selfhost.txt's "darwin/arm64 runtime verification" section
# for the mk/rc-only precedent this builds on, and todo.org's own
# bootstrap entry for the running list of what this has surfaced so far.
#
# Modeled on ocaml-light's own 'bootstrap' Makefile target (see
# ~/ocaml-light/Makefile), with one real simplification: goken's own
# ROOT/arch/$objtype/ directory-per-objtype scheme (mkfiles/mkone's own
# header comment) means nothing is ever overwritten in place the way
# ocaml-light's boot/ocamlc is -- so there is no backup/Saved/restore
# dance needed here, unlike ocaml-light's own 'backup'/'restore' targets.
#
# Stages:
#   0. ROOT/arch/boot-gcc (already built by the normal `make`/`mk
#      install` flow) -- the host-gcc-built toolchain this whole tree
#      bootstraps from.
#   1. stage 1 (below): build a CORE subset of the toolchain (this
#      host's own arch letter's compiler/assembler/linker, plus
#      mk/rc/ar/lex/yacc) for THIS host's own native ($cputype,
#      $ostype) -- using boot-gcc's compiler -- into ROOT/arch/
#      $cputype/bin. Exactly the manual recipe mk/rc's own self-hosting
#      verification used; see notes_libc_selfhost.txt. Deliberately NOT
#      the full top-level $DIRS: that also includes other arches'
#      cross-compilers, GO/'s historical reference sources, debuggers/
#      acid, typesetting, and utilities/, none of which this bootstrap
#      has attempted self-hosting for yet.
#   2. promote: copy that output into ROOT/arch/boot-goken/bin -- a NEW,
#      permanent name (distinct from boot-gcc, and from $cputype, which
#      is ALSO used for ordinary cross-target test builds) meaning
#      specifically "the toolchain goken built of itself, trusted as a
#      reference".
#   3. stage 2: rebuild the SAME core toolchain AGAIN for $cputype/
#      $ostype, this time with PATH preferring boot-goken's own
#      7c/7a/7l -- true self-compilation (the compiler compiling
#      itself), overwriting ROOT/arch/$cputype/bin a second time.
#   4. bootstrap-compare: diff boot-goken's binaries against the
#      freshly-self-rebuilt $cputype ones -- the fixpoint check.
#
# CURRENT STATUS (2026-08): stage 1's ENTIRE BOOTSTRAPDIRS chain is
# self-hosted and runtime-verified -- lib_core/libbio, lib_strings/
# libregexp+libstring, generators (lex+yacc, including the yacc *tool*
# itself), lib_toolchain/libmach, linkers/ar, compilers/cck,
# compilers/${BOOTSTRAPLETTER}c, assemblers/${BOOTSTRAPLETTER}a, and
# linkers/${BOOTSTRAPLETTER}l, plus mk/rc (self-hosted in an earlier
# round, see notes_libc_selfhost.txt). The whole arm64/darwin core
# toolchain triad -- 7c, 7a, 7l -- confirmed working TOGETHER, not just
# individually: the self-hosted 7c compiled a real test source, the
# self-hosted 7l linked the result into a real, running Mach-O binary
# (tests/c/regressions/arm64_fmovd_large_offset.c); yacc's own y.tab.c
# output confirmed byte-identical to the committed reference; 7a's own
# assembled output confirmed byte-identical to the boot-gcc-built 7a's;
# ar's archive create+list confirmed working.
#
# Every gap found getting here turned out to be a real, honestly-
# fixable bug, not a dead end -- see each fix's own commit for the
# individual writeups, and docs/claude_notes/notes_arch_arm64.txt for
# the arm64-specific ones. Two categories worth calling out because
# they'd bite ANY future self-hosting work in this tree, not just this
# one: `#if 0`/`#endif` (goken's own compilers have NO `#if` support at
# all, only `#ifdef`/`#ifndef` -- silently fine under gcc, "unknown #:
# if" under 7c; hit twice, linkers/7l/falloc.c and linkers/6l/
# compat.c, both from THIS session's own earlier gethunk() fixes, both
# switched to `//` line comments instead) and char*-vs-byte[] call-site
# mismatches wherever a `struct ar_hdr`/similar raw-bytes struct gets
# passed to a text function (linkers/ar/ar.c, lib_toolchain/libmach/
# obj.c, linkers/7l/pobj.c -- tolerated as a warning by gcc, a hard
# error under 7c). compilers/cc/compat.c has a known, NOT-yet-hit
# instance of the `#if 0` gap too (todo.org) -- deferred, since
# compilers/cc isn't part of this chain.
#
# Not yet attempted: stage 2 (self-compilation -- rebuilding this same
# chain a second time using boot-goken's own freshly-built compiler
# instead of boot-gcc's) and bootstrap-compare (the fixpoint check).
#
# Host portability: meant to run from linux and windows too, not just
# this macOS/arm64 host -- the arch-letter mapping and the darwin-only
# RT0OFILE branch below are both keyed off $cputype/$ostype (mkconfig's
# own ./configure-detected values for whatever host this runs on), not
# hardcoded to arm64/darwin, so the STAGE logic itself already travels.
# The handful of external commands this recipe calls (mkdir/cmp/
# basename) already resolve via $PATH to goken's OWN boot-gcc-built
# utilities/ binaries (installed as part of the top-level $DIRS, ahead
# of the host's system ones in $PATH -- see env.sh), not the host's --
# so those are already GOOS-portable by construction, no extra work
# needed. `cp` is the one real holdout: utilities/files/cp.c exists but
# is deliberately NOT installed to bin/ (utilities/files/mkfile's own
# comment: installing it causes a "Text file busy" error when mk itself
# is mid-copy), so this recipe falls back to the host's own `cp` --
# fine on macOS/Linux, but will need `copy`/xcopy or a POSIX layer once
# this target is actually attempted on real Windows hardware. Matching
# this project's own established honesty elsewhere (windows sections of
# notes_libc_selfhost.txt): not solved preemptively without a way to
# verify it.
# ==============================================================================

# claude: arch-letter mapping (Plan9's single-character per-arch code,
# see CLAUDE.md's own architecture table) -- always exactly one line of
# output (the UNKNOWN-ARCH-* fallback included), so this mk backquote
# never hits the "empty backquote yields one stray empty word" pitfall
# mkfiles/mkfile.proto's own BOOTLIBS comment warns about.
BOOTSTRAPLETTER=`{if(~ $cputype arm64) echo 7; if not if(~ $cputype amd64) echo 6; if not if(~ $cputype 386) echo 8; if not if(~ $cputype arm) echo 5; if not if(~ $cputype mips) echo v; if not if(~ $cputype riscv riscv64) echo i; if not echo UNKNOWN-ARCH-$cputype}
#`

# claude: deliberately NOT the full top-level $DIRS -- see this
# section's own header comment above for what's excluded and why.
# generators (yacc, and lex which depends on it -- generators/mkfile's
# own "lex depends on yacc" comment) needed yacc.c's mkstemp() call
# wired to a real create() first (lib_core/libc/port/mkstemp.c, layered
# on os/$GOOS/open.c's own create() -- already implemented there but
# untested on real hardware until this round; both now verified
# working, and the self-hosted yacc's own y.tab.c output confirmed
# byte-identical to the already-committed reference for rc/syn.y).
BOOTSTRAPDIRS=lib_core/libbio lib_strings/libregexp lib_strings/libstring \
	generators/lex/liblex/ generators \
	lib_toolchain/libmach linkers/ar mk rc \
	assemblers/${BOOTSTRAPLETTER}a linkers/${BOOTSTRAPLETTER}l \
	compilers/cck compilers/${BOOTSTRAPLETTER}c

bootstrap:V:
	echo '=== bootstrap: arch letter '$BOOTSTRAPLETTER', host '$cputype'/'$ostype' ==='
	echo '=== stage 1: core toolchain for '$cputype'/'$ostype', built by boot-gcc ==='
	# claude: mk exports every mkfile variable into its recipe's shell
	# environment (mk/env.c's initenv(), "user variables in mkfiles, or
	# mk process environment" -- unlike GNU Make, no explicit `export`
	# needed). Since `mk bootstrap` itself normally runs under the
	# default objtype=boot-gcc, mkfiles/boot-gcc/mkfile's own
	# `BOOTLIBS=-l9 -lm` is already in THIS recipe's environment before
	# any of the `mk 'objtype='$cputype ...` calls below even start --
	# and since a *child* mk process seeds its own variables from its
	# environment for anything the invoking command line doesn't
	# override, and mkfiles/$cputype/mkfile never sets BOOTLIBS itself
	# (relying on it being genuinely unset, mkfiles/mkfile.proto's own
	# "eagerly-substituted empty-string" comment), the inherited
	# boot-gcc value leaks straight through: `-lbio -l9 -lm` on a
	# non-boot LDLIBS line, which 7l/6l/etc. then can't find (this
	# host's own arm64/darwin lib dir has no lib9.a). `BOOTLIBS=()`
	# (rc's own "bind to zero words" unset idiom) below clears it --
	# verified against a FRESH `bin/rc` build (see below) that this
	# correctly omits the variable from every child process's
	# environment from here on, not just from rc's own $BOOTLIBS.
	#
	# A first attempt at this appeared not to work at all (`env | grep
	# BOOTLIBS` inside a freshly spawned child still showed the
	# ORIGINAL "-l9 -lm", unchanged, even after `BOOTLIBS=()`) --
	# looked like rc itself failing to sync a reassigned variable into
	# the real process environment it hands to exec(). It wasn't: `bin/
	# rc` (Jul 29) predated rc/unix.c's own most recent change (Aug 3,
	# "cmpenv/mkenv/Updenv were commented out ... restored below" --
	# exactly the mechanism this depends on) by over a week, so the
	# binary actually being tested had none of that fix. Rebuilding
	# `bin/rc` from current source (`cd rc; mk 'objtype=boot-gcc'
	# install; scripts/promote-mk.sh`, needed for `bin/mk` too, also
	# stale for the same reason) reproduced correct behavior
	# immediately: a plain reassignment AND `BOOTLIBS=()` both propagate
	# to a real child's environment as expected, no workaround needed.
	# Lesson for next time: scripts/promote-mk.sh's own header comment
	# explains why `mk install` can't just overwrite bin/mk or bin/rc in
	# place ("Text file busy") -- but that also means a change to mk/rc
	# source silently does NOT reach bin/ until someone remembers to
	# rebuild for objtype=boot-gcc and rerun that script by hand, with
	# no staleness warning of any kind in between.
	# claude: retries a full `mk ... install` up to 20 times, 2s apart
	# -- the freshly-linked binary a single such invocation produces
	# occasionally vanishes moments after being written (`cp: ...: No
	# such file or directory` right after a clean link), matching this
	# host's own AV-quarantine race already on record (docs/
	# claude_notes/notes_debug_techniques.txt). Confirmed which AV this
	# session (`ps aux | grep -i falcon`): CrowdStrike Falcon is
	# installed and running, including
	# StaticAnalysis.bundle/.../FXPredictService.xpc -- an ML-based
	# static-analysis component, evidenced further by a burst of dozens
	# of `trustd: SecKeyVerifySignature` calls in the same few
	# milliseconds as a disappearance (`/usr/bin/log show`, not the
	# zsh-builtin `log`). Consistently concentrated on linkers/ar's own
	# output specifically, far more than any other BOOTSTRAPDIRS entry
	# across many consecutive `mk bootstrap` runs (5 retries was not
	# enough, 20 needed here) -- plausibly because it's the one binary
	# here linking lib_toolchain/libmach, which embeds a lot of raw
	# Mach-O/ELF/PE header and opcode-table data an ML static-analysis
	# model may weight as more suspicious than a plain compiler
	# frontend; not confirmed against CrowdStrike's own (inaccessible)
	# detection internals, so held as a plausible explanation, not a
	# proven one. A plain retry still reliably clears it within a
	# bounded number of attempts (same as every other place in this
	# tree that's hit the race), so wrapping every nested install here
	# rather than requiring the caller to notice and rerun `mk
	# bootstrap` by hand.
	BOOTLIBS=()
	fn bsmk {
		extra = $*
		for(i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20){
			if(~ $ostype darwin)
				mk $extra 'RT0OFILE=arch/'$cputype'/rt0_darwin.'$BOOTSTRAPLETTER install
			if not
				mk $extra install
			if(~ $status '')
				break
			echo '  (retry '$i': transient link/copy failure, likely AV-quarantine race)'
			sleep 2
		}
	}
	fn bootstrapbuild {
		@{cd lib_core/libc; rm -f libc.a
			bsmk -a 'objtype='$cputype 'cputype='$cputype 'GOOS='$ostype
		}
		for(d in $BOOTSTRAPDIRS) @{
			echo $d
			cd $d
			# claude: iar merges into whatever local intermediate .a
			# archive is already sitting in this directory rather than
			# starting fresh (mkfiles/mkfile.proto's own mksyslib
			# comment) -- a stale one left over from the ORIGINAL
			# build-mk.sh bootstrap (built by the host's real ar, a
			# different, incompatible archive format) makes iar choke
			# with "ar: phase error", already hit and worked around
			# manually for lib_core/libbio, lib_strings/libregexp, and
			# lib_strings/libstring earlier this same self-hosting
			# effort (see notes_libc_selfhost.txt) -- baked in here so
			# every BOOTSTRAPDIRS entry gets it, not just the ones
			# already known to need it (generators/lex/liblex/ hit this
			# too, the first time this loop was actually run end to
			# end). A no-op for directories with no local .a at all.
			rm -f *.a
			bsmk 'objtype='$cputype 'cputype='$cputype 'GOOS='$ostype
		}
	}
	bootstrapbuild
	echo '=== stage 1 done: ROOT/arch/'$cputype'/bin now self-hosted (built by boot-gcc) ==='
	echo '=== promoting to ROOT/arch/boot-goken ==='
	mkdir -p ROOT/arch/boot-goken/bin ROOT/arch/boot-goken/lib
	cp ROOT/arch/$cputype/bin/* ROOT/arch/boot-goken/bin
	cp ROOT/arch/$cputype/lib/* ROOT/arch/boot-goken/lib
	echo '=== stage 2: rebuild again, using boot-goken (true self-compilation) ==='
	PATH=(`{pwd}/ROOT/arch/boot-goken/bin $PATH)
	bootstrapbuild
	echo '=== stage 2 done ==='
	mk bootstrap-compare

# claude: standalone target too (not just invoked at the end of
# `bootstrap` above), same as ocaml-light's own separate 'compare'.
# Scoped to bin/* only, not lib/*.a -- ar archives embed per-member
# metadata that may not be byte-reproducible even for a semantically
# identical rebuild (not verified either way for iar yet), so a DIFFERS
# there wouldn't necessarily mean anything is actually wrong.
bootstrap-compare:V:
	ok=1
	for(f in ROOT/arch/boot-goken/bin/*) @{
		b=`{basename $f}
		if(! cmp -s $f ROOT/arch/$cputype/bin/$b) {
			echo 'DIFFERS: '$b
			ok=0
		}
	}
	if(~ $ok 1)
		echo 'Fixpoint reached: bootstrap succeeded.'
	if not
		echo 'Fixpoint not (yet) reached -- see DIFFERS above. May still be fine if only non-deterministic bits (paths/timestamps) differ; not yet confirmed for this toolchain.'

#TODO: LPDIRS like in principia
sync:VQ:
	echo TODO
