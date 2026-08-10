// wasm counterpart of linux_amd64.s/macos_amd64.s/windows_amd64.s:
// hand-written TEXT'd write()/_start, adapted to wasm's own runtime
// conventions instead of a real OS's syscall ABI. See docs/notes_wasm.txt.
//
// Two things a real arch's write()/_main never need, both wasm-only:
//
//   - write() itself: there is no raw "write" syscall in wasm. stdout
//     goes through a WASI host import, wasi_snapshot_preview1.fd_write
//     (fd, iovs_ptr, iovs_len, nwritten_ptr) -> errno (see
//     tests/s/mini/hello_wasm.s's own comment) -- a *different* shape
//     than libc's write(fd, buf, n), so this wraps one into the other,
//     building the one-element iovec struct {ptr,len} fd_write expects
//     into a private scratch global (iov/nwritten below), never
//     touching the caller's own stack frame (ec has no address-taken-
//     local/shadow-stack support yet -- see docs/notes_wasm.txt's
//     "Open questions" -- so this wrapper is careful not to need one).
//
//   - SIGNATURE: wasm's call instruction is structurally typed (the
//     validator checks a call site's argument count/types against the
//     callee's declared function type), unlike a real ISA's CALL,
//     which doesn't care. ec emits this automatically for its own
//     compiled functions (ASIGNATURE, right after ATEXT -- see
//     compilers/ec/txt.c), but a hand-written .s TEXT has no such
//     annotation by default (see e.out.h's ASIGNATURE comment) and
//     would otherwise get the void/no-args signature every other
//     hello_*.s's parameterless _start uses. write() is called from
//     ec-compiled C with three real int arguments, so it needs its own
//     real signature spelled out explicitly.
TEXT	write(SB), $0
SIGNATURE	write(SB), $"WWWV"

	CONSTW	$iov+0(SB)
	LOCALGET	LOCAL(1)	// buf
	STOREW	$0

	CONSTW	$iov+4(SB)
	LOCALGET	LOCAL(2)	// n
	STOREW	$0

	LOCALGET	LOCAL(0)	// fd
	CONSTW	$iov(SB)
	CONSTW	$1
	CONSTW	$nwritten(SB)
	CALL	fd_write(SB)
	DROP			// discard the i32 errno result

	// claude: genuinely void (SIGNATURE above is "WWWV", not "WWWW") --
	// an earlier version pushed LOCAL(2) here to "pretend success" with
	// a real i32 result, matching hellowrite_wasm.c's own *local*
	// `extern int write(...)` prototype. But minilibc.h -- the
	// prototype print_nofloat_no64.c's vprintf()/printf() actually
	// compile against -- declares `extern void write(...)`, and ec's
	// cgen() (cgen.c's OFUNC case) trusts the caller's own C-level
	// declared type to decide whether a bare call-statement needs an
	// ADROP, not this function's *real* wasm signature: a void-declared
	// call is assumed to push 0 results, so no drop is ever emitted for
	// it. With write() actually returning 1 value here, every void-
	// prototyped call left a genuine, un-dropped value behind -- masked
	// wherever something else's own codegen happened to consume/absorb
	// it, but a hard wasm validation failure ("expected 0 elements ...
	// found 1") for any function whose write() call was its last
	// statement (e.g. print_nofloat_no64.c's own ·printbool()/vprintf()).
	// Matching the SIGNATURE to what minilibc.h actually promises is the
	// real fix -- hellowrite_wasm.c's own separate `int`-returning
	// prototype was simply never true of the underlying wasm function
	// either way (nothing has ever read the "pretend success" value).
	RET

GLOBL	nwritten(SB), $4
GLOBL	iov(SB), $8

// -------------------------------------------
// _start: WASI's own entry-point convention (not "_main" -- there is
// no separate arg/env setup step to do first the way a real OS's
// process-startup thunk needs; see tests/s/mini/hello_wasm.s).
// -------------------------------------------
TEXT	_start(SB), $0
	CALL	main(SB)
	RET
