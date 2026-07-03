// Exercise #define macro expansion: the assembler pushes the expansion
// as an Io buffer with no file (f == FD_NONE); popping it must not emit
// a history record. A missing guard in filbuf() made the principia 8a
// emit one spurious AHISTORY per macro use (caught on the principia
// corpus, e.g. lib_core/libc/386/atom.s).

#define NOP2	BYTE $0x90; BYTE $0x90

TEXT	_start+0(SB), $0
	NOP2
	MOVL	$1, AX
	XORL	BX, BX
	NOP2
	INT	$0x80
