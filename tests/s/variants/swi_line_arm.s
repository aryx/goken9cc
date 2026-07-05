// Minimal repro of the arm line-number mismatch: the line recorded in
// the .5 record used to be read (lineno) when yacc REDUCES the
// instruction rule, which happens before or after consuming the
// newline lookahead depending on the LALR tables (yacc default
// reductions). The principia 5a grammar reorganization gave SWI its
// own slim rule (LSWI cond gen) that reduces without lookahead, so a
// SWI recorded its true line while every other opcode recorded
// line+1; the kencc grammar (generic LTYPE6 class) always recorded
// line+1. Fixed in both lineages by recording stmtline, captured by a
// yylex wrapper on every token except the end-of-statement ';', which
// is deterministic and records the true line (see assemblers/5a/lex.c
// and assemblers/5ak/lex.c).
//
// The MOVW/SWI pair on consecutive lines is the minimal trigger: the
// two instructions used to get the same line number from the
// principia 5a.
TEXT _start(SB), $0
	MOVW	$1, R7
	SWI	$0
