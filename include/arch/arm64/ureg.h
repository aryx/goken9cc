
struct Ureg {
	uint64	r0;
	uint64	r1;
	uint64	r2;
	uint64	r3;
	uint64	r4;
	uint64	r5;
	uint64	r6;
	uint64	r7;
	uint64	r8;
	uint64	r9;
	uint64	r10;
	uint64	r11;
	uint64	r12;
	uint64	r13;
	uint64	r14;
	uint64	r15;
	uint64	r16;
	uint64	r17;
	uint64	r18;
	uint64	r19;
	uint64	r20;
	uint64	r21;
	uint64	r22;
	uint64	r23;
	uint64	r24;
	uint64	r25;
	uint64	r26;
	uint64	r27;
	uint64	r28;	/* sb */
	uint64	r29;
	union {
		uint64	r30;
		uint64	link;
	};
	uint64	sp;
	uint64	pc;	/* interrupted addr */
	uint64	psr;
	uint64	type;	/* of exception */
};
