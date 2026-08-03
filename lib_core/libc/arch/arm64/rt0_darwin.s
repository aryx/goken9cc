// Process startup glue for darwin/arm64 -- unlike arch/arm64/rt0.s
// (linux/arm64, and every other GOOS/arch this project builds), this
// one is genuinely GOOS-specific, not shared, for the same reason
// arch/amd64/rt0_darwin.s is: dyld's LC_MAIN calling convention hands
// argc/argv to the entry point in registers (X0/X1 here), not on the
// stack the way a raw Linux ELF entry does. Unlike amd64 though, this
// happens to already match 7c's own register-based first-argument
// convention for X0 (confirmed via 7c -S: main's own argc parameter
// arrives in R0 exactly the way dyld already delivers it) -- so this
// file needs no bridging at all, just the SB setup every arm64 _main
// needs regardless of GOOS. See lib_core/libc/mkfile's RT0OFILE
// comment for why this second file exists and how it's selected.
//
// claude: this file didn't exist before -- arch/arm64/rt0.s used to be
// this exact code (no argc/argv bridge, since none was needed for
// EITHER linux or darwin at the time). Once linux/arm64 needed a real
// bridge (previously it called main with no argument setup at all --
// see arch/arm64/rt0.s's own comment), that bridge started reading the
// raw stack pointer unconditionally, which is wrong for darwin: it
// overwrites the X0/X1 dyld already set correctly with garbage read
// from the Mach-O stack (which has no Linux-shaped argc/argv layout at
// all). Split out here, verbatim, to restore darwin/arm64's previously-
// correct (bridge-free) behavior without regressing linux/arm64's new
// fix.
TEXT _main+0(SB), $0
	MOV	$setSB(SB), R28
	BL	main+0(SB)
	MOV	$0, R0
	BL	exit+0(SB)
loop:
	B	loop
