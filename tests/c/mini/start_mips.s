TEXT _start(SB), 1, $0
	MOVW $setR30(SB), R30
	JAL main(SB)
