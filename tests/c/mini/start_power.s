TEXT _start(SB), $0
	// static base (gp = REGSB = R2), needed for the global string
	// references qc generates in main -- same role as setR30 in
	// start_mips.s / setSB in start_alpha.s.
	MOVW $setSB(SB), R2
	BL main(SB)
