
TEXT _start+0(SB), $0
	// without this only "Hello C world" is printed and not "It works!"
        // (like for start_arm.s)
	MOV    $setSB(SB), R28
	BL main+0(SB)

