// Process startup glue for linux/amd64, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_amd64.s's _main
// block). Unlike arm64/mips, amd64 needs no explicit SB/global-pointer
// register setup here (confirmed against 6c -S output: 6c-generated
// code never relies on one) -- just call user main(), and fall back to
// exit(0) if main() returns without calling exit() itself.
//
// note: unlike arm64/mips, amd64 has no register-passed argument at
// all (confirmed against 6c -S: every argument, including a callee's
// first, is written to the stack by the *caller* before the call) --
// so exit's code argument goes at 0(SP), not into some register.
//
// claude: argc/argv bridge. At _main (the ELF entry point -- see
// linkers/6l/obj.c's INITENTRY="_main"), SP is exactly what the Linux
// kernel handed the process at execve(): 0(SP) holds argc, 8(SP)
// onward holds argv[0], argv[1], ..., a nil, then envp, then auxv (the
// same layout Go's own 2010-era GO/pkg/runtime/amd64/asm.s _rt0_amd64
// bridges -- everything else in that file, TLS/g0/m0/scheduler setup,
// is Go-runtime scaffolding this libc has no equivalent of and doesn't
// need). main(int argc, char *argv[]) wants both args on the stack
// (per the note above), at whatever becomes its own 0(FP)/8(FP) once
// called -- but that is 0(SP)/8(SP) *right before* CALL, which is the
// exact memory CALL's own return-address push would clobber if left
// where the kernel put it, and writing there also self-clobbers argc/
// argv[0] before they are safely captured. So: read argc into AX and
// compute the argv pointer into BX first (both still valid, unwritten
// memory), then SUBQ to open a fresh 16-byte area *before* writing
// them, exactly mirroring the shape (if not the TLS motive) of Go's
// own "copy arguments forward" comment.
//
// claude: unlike arm/arm64/mips/riscv/riscv64 (whose BL/JAL don't
// touch the stack at all), this bridge is not strictly load-bearing on
// amd64: a bare "CALL main" with zero setup happens to already land
// argc/argv at main's own FP+0/FP+8, because CALL's own return-address
// push goes to SP-8 without disturbing SP+0/SP+8 -- see
// arch/amd64/rt0_darwin.s's comment, which independently makes the
// same observation for why *that* file needs a real bridge (dyld hands
// argc/argv in registers, not on the stack) while this one previously
// didn't. Added anyway for the same explicit, no-coincidences shape as
// every other arch's rt0.s, and because relying on the return-address
// push landing exactly below argv[0] without corrupting it is a fragile
// invariant to leave undocumented.
TEXT _main+0(SB), $0
	MOVQ	0(SP), AX	// argc
	LEAQ	8(SP), BX	// argv
	MOVQ	BX, _mainargv+0(SB)	// see port/mainargs.c
	MOVQ	AX, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment
	SUBQ	$16, SP
	MOVQ	AX, 0(SP)
	MOVQ	BX, 8(SP)
	CALL	main+0(SB)
	MOVL	$0, (SP)
	CALL	exit+0(SB)
loop:
	JMP	loop
