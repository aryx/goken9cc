TEXT _start(SB), $0
#ifndef arm_
	// needed for 5l (and 5l__); without this only "Hello C world"
	// is printed and not "It works!"
	MOVW $setR12(SB), R12
#endif
	BL main(SB)
