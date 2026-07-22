// Regression test for 6l -H6 (macOS Mach-O)'s dyld rebase info
// (linkers/lk/macho.c's machorebase()) failing to recognize a pointer
// stored in initialized data when the pointer initializer comes from
// 6c rather than hand-written assembly.
//
// 6c represents a symbolic DATA initializer like "dig" below with
// to.type=D_ADDR and the real type (D_STATIC/D_EXTERN) boxed in
// to.index (see vaddr()'s own "if(t == D_ADDR) t = a->index;"
// unwrapping in linkers/6l/span.c), not directly as D_CONST like
// machorebase() originally checked for (a convention apparently
// written for a different ?l's data model, since it works as-is for
// 7l). Without the fix, dyld never adjusts this pointer for ASLR, so
// reading through it would fault or return the pre-slide address once
// the image is actually loaded at a randomized base.
//
// Golden-diffed here (no macOS/darling execution available in this
// environment) against a linked -H6 exe: catches a regression either
// in that D_ADDR-unwrapping fix, or in the -z (position-independent
// code) compiler-side work (hardconst()/doindex()/naddr() in cgen.c/
// txt.c) that produces the RIP-relative reference to "dig" itself.
//
// expected/pievar_amd64.exe was regenerated after the REGARG=-1 fix
// (see include/6.out.h): digit(int i)'s single int parameter used to
// be read out of BP under the old REGARG=D_BP convention (never
// actually spilled there by any real caller, so effectively garbage);
// it's now correctly read off the stack like every other argument in
// this project, changing digit()'s compiled bytes. See regarg_amd64.c
// for the bug itself.

char *dig = "0123456789";

char
digit(int i)
{
	return dig[i];
}
