// float_const_oob: found via internal cross-arch consistency while
// reviewing the mips/riscv toolchains (not diff-derived from another
// repo -- see plan9front.txt's riscv section for the ones that are).
//
// linkers/il/asm.c and linkers/vl/asm.c's datblk() (writing a D_FCONST
// global/static initializer into the data segment) had, for the 4-byte
// (float, as opposed to 8-byte double) case:
//
//     fl = ieeedtof(p->to.ieee);   // fl is declared `int32 fl` -- 4 bytes
//     cast = (char*)&fl;
//     for(; i<c; i++)
//         buf.dbuf[l] = cast[fnuxi8[i+4]];   // fnuxi8[4..7] = indices 4-7
//
// fnuxi8[] is an 8-entry byte-order table where indices 0-3 apply to a
// 4-byte quantity and 4-7 to the *upper* half of an 8-byte one (see
// nuxiinit() in each linker's obj.c). Using "[i+4]" here reads 4 bytes
// past the end of the 4-byte `fl` -- a stack buffer overread. Every
// other backend (5l/5lk/6l/7l/8l/8lk) already uses the correct
// "cast[fnuxi8[i]]" form here; il and vl were the only two with the bug.
//
// Confirmed with a real qemu run before landing the fix: the stored bits
// of the float below didn't match its known-correct IEEE-754 encoding.

extern void exit(int);

float pi = 3.14f;

void
main(void)
{
	int bits;
	float *p;

	p = &pi;
	bits = *(int*)p;
	exit(bits != 0x4048f5c3);
}
