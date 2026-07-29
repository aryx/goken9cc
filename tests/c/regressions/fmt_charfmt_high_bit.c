// claude: fmt_charfmt_high_bit: found while unblocking benchs/compcert/
// mandelbrot.c -- `putc(byte_acc, stdout)` (via include/APE/stdio.h's
// `#define putc(c, stream) print("%c", (c))`) silently wrote *nothing*
// whenever byte_acc's value had the high bit set (>= 128 as unsigned,
// negative as signed char) -- print() returned normally, no error, no
// byte written. A 4096-byte tight-loop repro (`putc((char)(i&0xff),
// stdout)` for i in 0..4095) lost exactly the bytes for values 128-255
// every 256-iteration cycle: contiguous runs of 0..127 came through
// correctly.
//
// Root cause: lib_core/libc/fmt/dofmt.c's __charfmt() (the %c
// handler) delegated to __fmtcpy(), which is written for copying
// UTF-8 *text* (correct for its other callers, %s/%q/float-formatting
// suffixes/etc): any byte >= Runeself (0x80) is treated as the
// leading byte of a multi-byte UTF-8 sequence and decoded via
// chartorune(). For a single raw byte handed to %c, fullrune() (only
// one byte ever available) correctly reports "not a complete
// sequence", so __fmtcpy() took its `else break;` path -- writing
// nothing at all, with no error returned. %c's actual contract in
// this codebase (see include/APE/stdio.h's own comment on putc()) is
// "write exactly one raw byte, whatever its value" -- UTF-8 rune
// formatting is %C's (__runefmt()) job, a separate, already-correct
// function a few lines below __charfmt() in the same file. Fixed by
// having __charfmt() write the byte directly via the FMTCHAR macro
// (already used elsewhere in dofmt.c for plain non-rune output, e.g.
// numeric padding), bypassing __fmtcpy()'s UTF-8 decoding entirely.
//
// This is a runtime-library bug, not a compiler bug -- it affects
// every program using %c/putc() with a byte value >= 128, on any
// architecture (the fmt/ sources are shared, not per-arch). Uses
// snprint() (formats into a memory buffer, no syscall needed) rather
// than actually writing to a file descriptor, since it goes through
// the exact same dofmt()/__charfmt() code the real bug is in, without
// needing to capture stdout externally. See docs/claude_notes/
// notes_libc_selfhost.txt for the full writeup.
//
// Confirmed real: exit(1)/(2) below, not exit(0), before any fix.
//
// Not wired into this directory's plain `test:V:` (every other check
// there is compiler/linker-only and needs no libc.a rebuild) -- instead
// has its own `fmt_charfmt_high_bit.check_macos_arm64` mkfile target,
// modeled on tests/c/hello_libc/mkfile's test_macos_arm64 (same reason:
// needs a real, freshly-rebuilt darwin/arm64 libc.a, run natively since
// Mach-O only runs on real macOS -- see that mkfile's own comment for
// why the rebuild has to be forced with `mk -a`). A linux/qemu
// equivalent would be straightforward to add the same way (rebuild
// linux/arm64 libc.a, -H7, qemu-runner) whenever this is run somewhere
// qemu is available:
//   7c -c fmt_charfmt_high_bit.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o fmt_charfmt_high_bit.exe -E _main \
//      fmt_charfmt_high_bit.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./fmt_charfmt_high_bit.exe; echo $?

typedef unsigned char uint8_t8;

extern int snprint(char *, int, char *, ...);
extern void exit(int);

void
main(void)
{
	char buf[8];
	int n;

	n = snprint(buf, sizeof(buf), "%c%c%c", (char)65, (char)200, (char)66);
	if (n != 3)
		exit(1);	// the bug: the 200 byte wrote nothing, n==2
	if ((uint8_t8)buf[0] != 65 || (uint8_t8)buf[1] != 200 || (uint8_t8)buf[2] != 66)
		exit(2);
	exit(0);
}
