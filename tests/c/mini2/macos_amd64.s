// macOS (XNU) amd64 syscall stubs. Mirrors linux_amd64.s but with the
// Darwin ABI: the BSD syscall number is OR'd with the 0x2000000 Unix
// class bit and follows BSD numbering (write=4, exit=1); the trap is
// SYSCALL. See docs/notes_macos.txt.
//
// fd and count are C ints, so they are loaded with MOVL (32-bit): the
// caller stores only 32 valid bits and a 64-bit load would pick up
// stale stack garbage in the upper half, making write() fail with
// EINVAL -- the exact bug that surfaced on macOS 26 (Tahoe), see
// ../mini/xwrite_macos_amd64.s. buf is a pointer, loaded with MOVQ.
TEXT	_main(SB), $0
	CALL	main(SB)

TEXT	panic(SB), $0
	// syscall: exit(0)
	MOVQ	$(0x2000000+1), AX
	XORQ	DI, DI
	SYSCALL

TEXT	exit(SB), $0-8
	MOVL	8(SP), DI		// status (int)
	MOVQ	$(0x2000000+1), AX	// exit
	SYSCALL
	RET

TEXT	write(SB), $0-24
	MOVL	8(SP), DI		// fd (int)
	MOVQ	16(SP), SI		// buf (pointer)
	MOVL	24(SP), DX		// count (int)
	MOVQ	$(0x2000000+4), AX	// write
	SYSCALL
	RET
