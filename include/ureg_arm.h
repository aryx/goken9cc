
struct Ureg {
	uint	r0; // REGARG0, REGRET
	uint	r1;
	uint	r2;
	uint	r3;
	uint	r4;
	uint	r5;
	uint	r6;
	uint	r7;
	uint	r8;
	uint	r9;
	uint	r10;
	uint	r11;
	uint	r12; // REGSB
	uint	r13; // REGSP
	uint	r14; // REGLINK

	uint	link; // alias for R14
	uint	type;
	uint	psr;
	uint	pc; // alias for R15 (alt: use union)
};
