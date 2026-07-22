#!/bin/bash
# Regression tests for lib_toolchain/libmach's consumers: linkers/ar (iar),
# debuggers/acid, machines/5i, machines/vi.
#
# These four all link against lib_toolchain/libmach (-lmach). iar was
# always on it; acid/5i/vi were switched over from the go-era
# src/libmach (-lmach_) when lib_toolchain/libmach's map.c/sym.c/
# access.c/machdata.c/executable.c/elf.h/7.c/v.c/vdb.c/vcodas.c/linux.c
# were ported in from src/libmach (see lib_toolchain/libmach/mkfile
# history). The switch is easy to get wrong silently: `mk`/`make test`
# only check that these tools *compile and link*, not that libmach's
# executable-header decoding actually works at runtime -- e.g.
# lib_toolchain/libmach's old executable.c had no ELF e_machine==ARM
# case at all (only Plan9/Inferno "boot image" recognition), so acid
# linked fine but failed with "can't decode file header" on every real
# ELF binary until executable.c/elf.h/macho.h/bootexec.h were ported
# in too. These tests pin that down so it can't regress unnoticed.

set -e

IAR=${IAR:-iar}
ACID=${ACID:-acid}
FIVEI=${FIVEI:-5i}
VI=${VI:-vi}

PASS=0
FAIL=0

ok() { echo "PASS: $1"; PASS=$((PASS + 1)); }
bad() { echo "FAIL: $1"; FAIL=$((FAIL + 1)); }

# --- iar: still able to create and list an archive of real object
# files. Regression guard for the "don't break iar" requirement --
# lib_toolchain/libmach's obj.c/executable.c/etc are shared between
# iar and acid/5i/vi, so a change made for one must not break the
# other. ---
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT
cp "$(dirname "$0")"/../../lib_toolchain/libmach/*.o "$workdir"/ 2>/dev/null || true
if [ -f "$workdir/executable.o" ] && [ -f "$workdir/obj.o" ]; then
    (cd "$workdir" && "$IAR" r test.a executable.o obj.o >/dev/null 2>&1)
    listing=$("$IAR" t "$workdir/test.a" 2>&1)
    if [ "$listing" = "$(printf 'executable.o\nobj.o')" ]; then
        ok "iar: create + list archive of real .o files"
    else
        bad "iar: create + list archive of real .o files (got: $listing)"
    fi
else
    bad "iar: prerequisite .o files missing (build lib_toolchain/libmach first)"
fi

# --- acid: correctly identifies a real Linux ELF ARM executable's
# header. This is the exact bug: with the old (unported)
# lib_toolchain/libmach executable.c, this printed "can't decode file
# header" instead. ---
exe="$(dirname "$0")/../s/exit/exit_linux_arm.exe"
if [ -f "$exe" ]; then
    out=$(printf '\n' | timeout 5 "$ACID" "$exe" 2>&1 || true)
    if echo "$out" | grep -q "elf executable"; then
        ok "acid: decodes real Linux/ELF/ARM executable header"
    else
        bad "acid: failed to decode real Linux/ELF/ARM executable header (got: $out)"
    fi
else
    bad "acid: prerequisite $exe missing (run tests/s/exit's mkfile first)"
fi

# --- 5i: loads a real Plan9-format ARM executable (the format 5i
# actually emulates -- it is not an ELF/Linux debugger) without
# erroring, and exits cleanly on EOF. ---
plan9arm="$(dirname "$0")/../s/mini/hello_plan9_arm.exe"
if [ -f "$plan9arm" ]; then
    if out=$(timeout 5 "$FIVEI" "$plan9arm" < /dev/null 2>&1); then
        ok "5i: loads real Plan9/ARM executable header"
    else
        bad "5i: failed to load real Plan9/ARM executable header (got: $out)"
    fi
else
    bad "5i: prerequisite $plan9arm missing (run tests/s/mini's mkfile first)"
fi

# --- 5i: still correctly REJECTS a wrong-arch (mips) binary. Confirms
# the above pass is because the header was actually decoded and
# checked, not because 5i stopped validating the arch. ---
plan9mips="$(dirname "$0")/../s/mini/hello_plan9_mips.exe"
if [ -f "$plan9mips" ]; then
    if out=$(timeout 5 "$FIVEI" "$plan9mips" < /dev/null 2>&1); then
        bad "5i: accepted a wrong-arch (mips) executable (got: $out)"
    else
        ok "5i: rejects wrong-arch (mips) executable"
    fi
else
    bad "5i: prerequisite $plan9mips missing (run tests/s/mini's mkfile first)"
fi

# --- vi: loads a real Plan9-format MIPS executable without erroring. ---
if [ -f "$plan9mips" ]; then
    if out=$(timeout 5 "$VI" "$plan9mips" < /dev/null 2>&1); then
        ok "vi: loads real Plan9/MIPS executable header"
    else
        bad "vi: failed to load real Plan9/MIPS executable header (got: $out)"
    fi
else
    bad "vi: prerequisite $plan9mips missing (run tests/s/mini's mkfile first)"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
