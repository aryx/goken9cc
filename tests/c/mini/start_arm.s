TEXT _start(SB), 1, $0
	MOVW $setR12(SB), R12
	BL main(SB)
