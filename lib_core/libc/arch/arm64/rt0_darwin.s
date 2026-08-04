// Process startup glue for darwin/arm64 -- unlike arch/arm64/rt0.s
// (linux/arm64, and every other GOOS/arch this project builds), this
// one is genuinely GOOS-specific, not shared, for the same reason
// arch/amd64/rt0_darwin.s is: dyld's LC_MAIN calling convention hands
// argc/argv to the entry point in registers (X0/X1 here), not on the
// stack the way a raw Linux ELF entry does. This happens to already
// match 7c's own register-based first-argument convention for X0
// (confirmed via 7c -S: main's own argc parameter arrives in R0
// exactly the way dyld already delivers it) -- but only the FIRST
// argument is register-passed under 7c's calling convention (see
// docs/claude_notes/notes_debug_techniques.txt's "Cross-arch rt0.s
// bring-up" section); argv, main's SECOND parameter, is stack-passed
// like every other argument on every arch here, dyld's registers
// notwithstanding. See lib_core/libc/mkfile's RT0OFILE comment for why
// this second file exists and how it's selected.
//
// claude: this file didn't exist before -- arch/arm64/rt0.s used to be
// this exact code (no argc/argv bridge, since none was needed for
// EITHER linux or darwin at the time). Once linux/arm64 needed a real
// bridge (previously it called main with no argument setup at all --
// see arch/arm64/rt0.s's own comment), that bridge started reading the
// raw stack pointer unconditionally, which is wrong for darwin: it
// overwrites the X0/X1 dyld already set correctly with garbage read
// from the Mach-O stack (which has no Linux-shaped argc/argv layout at
// all). Split out here to restore darwin/arm64's own argc/argv source
// (dyld's registers, not the raw stack) without regressing linux/
// arm64's new fix.
//
// claude: the first version of this file stopped at storing R1 into
// _mainargv (for port/mainargs.c's getenv() use) and calling main()
// with only R0 set, on the theory that dyld's X0/X1 delivery meant "no
// bridging needed at all" -- true for argc (X0, register-passed) but
// NOT for argv (X0's neighbor is irrelevant; what matters is that 7c
// never reads a *second* argument from a register on any arch here).
// main()'s own prologue reads argv from the outgoing-argument stack
// slot the CALLER is expected to have written, exactly like
// arch/arm64/rt0.s's linux bridge already does below -- omitting it
// left that slot holding whatever this TEXT block's own auto-inserted
// non-leaf prologue (BL forces one, saving LR -- see the
// notes_debug_techniques.txt section above) happened to leave on the
// stack, undefined and only sometimes zero. Caught via lldb: argv[1]
// dereferenced a small integer instead of a pointer, EXC_BAD_ACCESS.
// Same "(new RSP)+16, not +8" outgoing-slot offset as linux/arm64's
// rt0.s, for the identical reason (its own callee's -- main's --
// prologue reservation); reuse it verbatim rather than re-derive it.
TEXT _main+0(SB), $0
	MOV	$setSB(SB), R28
	MOV	R1, _mainargv+0(SB)	// argv (dyld: R0=argc, R1=argv); see port/mainargs.c
	SUB	$32, RSP, RSP
	MOV	R1, 16(RSP)	// argv, main's own (stack-passed) 2nd argument
	BL	main+0(SB)
	MOV	$0, R0
	BL	exit+0(SB)
loop:
	B	loop
