/* mixaddr_arm: the compiler side.
 * &shared is emitted by 5c as a D_CONST operand carrying a name, whereas
 * mixaddr_arm.s refers to the same symbol via $shared(SB) = D_ADDR. The
 * two linkers must treat both as the same literal-pool constant (one
 * entry). See the mixaddr_arm rule in the mkfile.
 */
extern long shared[2];

long*
cfun(void)
{
	return &shared[0];
}
