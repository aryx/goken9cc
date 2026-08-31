// start_m68k.s -- Linux m68k process entry point for helloc.exe.
//
// A6 plays the same "static base" role kl's REGSB (R2) plays for
// sparc, needed for the string references 2c generates in main --
// see tests/s/hello_arch/hello_linux_m68k.s's comment for how a6base
// resolves bare sym(SB) references. Unlike sparc's REGSP (kl's own
// fake stack pointer, unrelated to any real hardware register), this
// backend's calling convention (2c -S on helloc.c: PEA args, BSR to
// call, RTS to return) uses the real hardware A7/%sp throughout, so
// there's no analogous "OS never initializes it" problem here -- BSR
// itself pushes the return address using the real stack pointer the
// kernel already set up.

TEXT _start(SB), $0
	MOVL $a6base(SB), A6
	BSR main(SB)
