// See hello_linux_amd64.s for the general shape this family of tests
// follows. wasm has no raw syscalls: stdout goes through a WASI host
// import, wasi_snapshot_preview1.fd_write(fd, iovs_ptr, iovs_len,
// nwritten_ptr) -> errno, wired up by el's -I flag (see linkers/el/
// l.h's Import comment) instead of a CALL to a TEXT'd helper.
//
// wasm's calling convention pushes arguments in order right before
// CALL, each with its own CONSTW (see e.out.h's opcode-design
// comment), rather than storing them at a known stack address the
// way SUBQ $16,SP / MOVQ AX,0(SP) do on amd64 -- there is no
// stack-relative argument area to build here.
//
// See docs/notes_wasm.txt for why this looks like 5a/8a/ia (SB/SP/FP,
// TEXT/GLOBL/DATA, one instruction per line) but does NOT reuse a
// single virtual MOV mnemonic the way those arches do: CONSTW/LOADW/
// LOCALGET/etc are each their own opcode, one per real wasm
// instruction, because wasm has no register to be "the other side"
// of a move the way every other arch's MOVW always has.
//
// See hello_wasm.wat for the exact same program in traditional
// WebAssembly Text (WAT) syntax, for side-by-side comparison. It
// looks quite different, but not because it computes anything
// differently -- ea/el and a normal wat2wasm-style toolchain end up
// emitting structurally the same module. WAT nests call arguments as
// sub-expressions and requires the imported function's type spelled
// out at the import site, because it's meant to be handwritten
// assembly for wasm specifically, with its own conventions -- not to
// share a family of syntax with a dozen other CPU architectures the
// way this project's .s files do. Neither is "the real one"; they're
// just two different assemblers' conventions for the same
// instruction set, the same way 5a's ARM assembly and gas's ARM
// assembly both assemble to the same opcodes.

// -------------------------------------------
// main procedure
// -------------------------------------------
TEXT	_start(SB), $0

	CONSTW	$1		// fd = 1 (stdout)
	CONSTW	$iov(SB)	// iovs_ptr: address of the iovec array
	CONSTW	$1		// iovs_len: one iovec
	CONSTW	$nwritten(SB)	// nwritten_ptr: where fd_write stores the count

	CALL	fd_write(SB)
	DROP			// discard the i32 errno result

	RET

// -------------------------------------------
// nwritten: scratch word fd_write writes its result into
// -------------------------------------------
GLOBL	nwritten(SB), $4

// -------------------------------------------
// msg: must split into 8-byte chunks, same convention as the other
// hello_*.s tests
// -------------------------------------------
DATA	msg+0(SB)/8, $"Hello, w"
DATA	msg+8(SB)/5, $"orld\n"
GLOBL	msg(SB), $13

// -------------------------------------------
// iov: a single struct iovec { ptr, len } pointing at msg
// -------------------------------------------
DATA	iov+0(SB)/4, $msg(SB)
DATA	iov+4(SB)/4, $13
GLOBL	iov(SB), $8
