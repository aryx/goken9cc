// Feature test for ARM64 LDAR/STLR (load-acquire/store-release) and
// LDXR/STXR/LDAXR/STLXR (load/store-exclusive) atomic instructions,
// fixed/wired end to end from 9front:
//   223e8ad00 7l, 7c: Remove STLP(W), finish LDAXR(W)/STLXR(W).
//   61f6376b1 7[al]: implement atomic register loads properly
// See plan9front.txt for the full writeup.
//
// Before this port: STLR/STLRB/STLRH/STLRW were grammared as the
// 3-operand exclusive-store form (LSTXR) instead of the correct
// 2-operand plain store-release form (LTYPE3), so `STLR R2, (R1)`
// failed to parse; ALDAR/ALDAXR/ASTLR/ASTLXR had no linkers/7l/optab.c
// entries at all, so even though 7a's lexer recognized the mnemonics,
// linking anything using them failed outright; and ALDAXRW/ASTLXP/
// ASTLXPW were encoded with the wrong instruction-size bit in
// linkers/7l/asmout.c's opload()/opstore(), which would have executed
// as the *wrong instruction* (e.g. ALDAXRW silently behaving like
// ALDARW, dropping the "exclusive" semantics an atomic CAS loop
// depends on) had they ever linked.
//
// This does a classic LDXR/STXR compare-and-retry increment loop (the
// same idiom lock code uses) wrapped in LDAR/STLR (acquire/release),
// then exit()s with the result.

TEXT _start(SB), $0
	MOV	$counter(SB), R1

	LDAR	(R1), R2	// acquire load, just to exercise it
	CBZ	R2, bad

retry:
	LDXR	(R1), R2
	ADD	$1, R2, R2
	STXR	R2, (R1), R3
	CBNZ	R3, retry

	STLR	R2, (R1)	// release store, just to exercise it

	MOV	(R1), R0	// exit(counter) -- must be 11
	MOV	$93, R8
	SVC	$0

bad:
	MOV	$1, R0
	MOV	$93, R8
	SVC	$0

GLOBL	counter(SB), $8
DATA	counter(SB)/8, $10
