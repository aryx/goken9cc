
TEXT _start+0(SB), 7, $0        // NOSPLIT | DUPOK | NOPROF
	// without this only "Hello C world" is printed and not "It works!"
        // (like for start_arm.s)
	MOV    $setSB(SB), R28
	BL main+0(SB)

