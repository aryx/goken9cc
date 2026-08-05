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
# CURRENT STATUS (2026-08): stage 1 gets through lib_core/libbio,
# lib_strings/libregexp+libstring, generators (lex+yacc, now including
# the yacc *tool* itself -- see lib_core/libc/port/mkstemp.c's own
# commit), lib_toolchain/libmach, and linkers/ar (all self-hosted and
# runtime-verified: yacc's own y.tab.c output confirmed byte-identical
# to the committed reference, ar's own archive create+list confirmed
# working), then mk/rc (already self-hosted and runtime-verified in an
# earlier round, see notes_libc_selfhost.txt), but does NOT yet reach
# compilers/${BOOTSTRAPLETTER}c: it hits a genuine, unfixed compiler
# bug there, not a portability gap -- 7c's own switch statement rejects
# a vlong controlling expression ("switch expression must be integer",
# compilers/7c/list.c:191). Every gap found along the way so far turned
# out to be a real, honestly-fixable portability gap, not a compiler
# bug: redundant `#include <ctype.h>` in three files (this project's
# own include/str/ascii.h, pulled in via libc.h, already covers
# everything they needed), and two missing libc primitives
# (mkstemp()/mktemp(), layered on the already-implemented-but-until-now-
# untested create()) plus several char*-vs-byte[] call-site mismatches
# in linkers/ar/ar.c and lib_toolchain/libmach/obj.c (fixed with
# memcmp()/memmove()/memchr() where the operation was genuinely
# byte-oriented, and explicit casts only where a byte[] field was
# genuinely being used as text). This target is deliberately left
# runnable-but-currently-failing: it documents the intended shape and
# gives a concrete, reproducible stopping point, rather than describing
# it only in prose.
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
	assemblers/${BOOTSTRAPLETTER}a linkers/${BOOTSTRAPLETTER}l compilers/${BOOTSTRAPLETTER}c

bootstrap:V:
	echo '=== bootstrap: arch letter '$BOOTSTRAPLETTER', host '$cputype'/'$ostype' ==='
	echo '=== stage 1: core toolchain for '$cputype'/'$ostype', built by boot-gcc ==='
	fn bootstrapbuild {
		@{cd lib_core/libc; rm -f libc.a
			if(~ $ostype darwin)
				mk -a 'objtype='$cputype 'cputype='$cputype 'GOOS='$ostype 'RT0OFILE=arch/'$cputype'/rt0_darwin.'$BOOTSTRAPLETTER install
			if not
				mk -a 'objtype='$cputype 'cputype='$cputype 'GOOS='$ostype install
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
			if(~ $ostype darwin)
				mk 'objtype='$cputype 'cputype='$cputype 'GOOS='$ostype 'RT0OFILE=arch/'$cputype'/rt0_darwin.'$BOOTSTRAPLETTER install
			if not
				mk 'objtype='$cputype 'cputype='$cputype 'GOOS='$ostype install
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
