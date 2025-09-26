TEXT _start(SB), 1, $0
	// needed for 5l_; without this only "Hello C world"
	// is printed and not "It works!"
	MOVW $setR12(SB), R12
	BL main(SB)
