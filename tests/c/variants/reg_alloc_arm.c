// Any function at all triggers a register-allocator bug in the
// principia arm C compiler (compilers/5c, installed as 5c___): ginit()
// reserves R9 and R10 (REGEXT and REGEXT-1) in reg[] to "keep two
// external registers", but the resvreg[] snapshot (memmove(resvreg,
// reg)) was taken *before* those two reservations, so at the end of
// every function gclean()'s "reg N left allocated" sanity check fires
// for R9 and R10 and the compile fails (exit 1). kencc's 5c takes the
// snapshot after the reservations, so it is fine.
//
// Cause: an LP chunk reorder in principia's 2018-01-17
// "compilers/5c/: mk sync" (dae86be2) moved the resvreg snapshot chunk
// ahead of the R9/R10 reservation chunk. Fixed in compilers/5c/txt.c
// ginit() by also marking resvreg[REGEXT] / resvreg[REGEXT-1].
//
// This was invisible until the arm corpus sweep because the principia
// arm compiler had never been run over real code before.

int
f(int x)
{
	return x + 1;
}
