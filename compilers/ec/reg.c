/*
 * ec/reg.c -- true no-op.
 *
 * claude: every other arch's reg.c (~1000+ lines) builds a flow graph
 * over the Prog list and recolors hot stack-resident variables into
 * leftover hardware registers -- pure optimization on top of already-
 * correct code (see docs/notes_wasm.txt). wasm has no register file
 * to spill into in the first place, so there is nothing for this pass
 * to do; codgen() (compilers/cck/pgen.c) still calls it unconditionally
 * once per function, so it has to exist, just as a no-op.
 */
#include "gc.h"

void
regopt(Prog *p)
{
	USED(p);
}
