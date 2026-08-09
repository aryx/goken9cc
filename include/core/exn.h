
// Poor's man exceptions using setjmp()/longjmp().

/* claude: goken (unlike gcc/clang) preserves NO registers
 * across a function call at all -- every Xc backend reloads whatever
 * it needs from the stack after any call, so a real jmp_buf here only
 * ever needs the two things a "return a second time" trick actually
 * requires: the stack pointer to rewind to, and the return address to
 * jump back to. Confirmed against principia's own 386/arm setjmp.s
 * (lib_core/libc/386/setjmp.s, arm/setjmp.s), which save exactly these
 * two words and nothing else -- ported in spirit, not copied, since
 * this project targets 5 more arches principia's own reference never
 * covered (arm64/mips/riscv/riscv64/amd64), each needing its own
 * asm written from this compiler's OWN empirically-confirmed
 * (`Xc -S` on a probe function, not assumed) calling convention --
 * see lib_core/libc/arch/$cputype/setjmp.s for each one's own comment.
 *
 * `jmp_buf` stays an ARRAY-of-one-struct typedef (the classic C
 * idiom), not a bare struct: lib_strings/libregexp/regcomp.c declares
 * `static jmp_buf regkaboom;` and passes it as `setjmp(regkaboom)`/
 * `longjmp(regkaboom, 1)` with no explicit `&`, which only type-checks
 * (and only actually passes a pointer, which setjmp()/longjmp()'s own
 * asm bodies require) if the array-decays-to-pointer rule applies.
 */
typedef struct Jmpbuf Jmpbuf;
struct Jmpbuf {
	uintptr	sp;	/* stack pointer to rewind to */
	uintptr	pc;	/* return address to jump back to */
};
typedef Jmpbuf jmp_buf[1];

extern  int     setjmp(jmp_buf);
extern  void    longjmp(jmp_buf, int);

// alt: in os/plan9/note.h
//extern  void    notejmp(void*, jmp_buf, int);
