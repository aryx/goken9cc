/* riscv (rv32) base types for goken's own toolchain (ic/ia/il).
 *
 * TODO jmp_buf/mpdigit/FCR-FSR
 *
 * va_start/va_arg were NOT copied as-is: verified
 * (and, for one case, corrected) against real ic -S output and actual
 * qemu-riscv execution below, since goken's own ic is a different,
 * independently-implemented compiler, not the same binary as
 * miller-riscv's.
 */

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;

typedef float float32;
typedef double float64;

typedef unsigned long uintptr;
typedef long intptr;

// bit-level double access for port/frexp.c (frexp/ldexp/modf) -- rv32
// is little-endian (like arm64/amd64/386, unlike big-endian mips), and
// ulong is 4 bytes here (matches uint32, no arm64-style size mismatch
// to worry about).
union FPdbleword {
	double x;
	struct {	/* little endian */
		unsigned long lo;
		unsigned long hi;
	};
};
typedef union FPdbleword FPdbleword;

/* Plan9-style va_list for riscv (rv32).
 *
 * Verified empirically against ic -S output (both a fixed-arg
 * function's own FP offsets and a variadic call site's caller-written
 * offsets) AND actual qemu-riscv execution of a real va_start/va_arg
 * sequence (not just static -S inspection -- see the riscv64 comment
 * below for why the latter mattered): like arm64/mips, only the
 * *first* named parameter arrives in a register (R8) -- a compiler-
 * generated function only spills it to its home slot (arg+0(FP)) if
 * its address is actually taken, so hand-written assembly (see
 * syscall/arch/riscv/svc.s) must read it from R8 directly. Every
 * *other* argument packs at its natural sizeof() width (like mips),
 * EXCEPT: unlike mips, any 8-byte-or-wider type (double, vlong) forces
 * the list pointer to round up to the next 8-byte-aligned address
 * first -- confirmed by a case that only fails without this rounding
 * (an int immediately followed by a double as the first two varargs).
 */

typedef char* va_list;

#define va_start(list, start) \
	(list = (sizeof(start) < 4 ? (char*)((int*)&(start) + 1) : (char*)(&(start) + 1)))

#define va_end(list)

#define va_arg(list, mode) \
	((sizeof(mode) < 8) ? \
		((list += 4), *(mode*)(list - 4)) : \
		(list = (char*)(((uintptr)list + 7) & ~7) + sizeof(mode), *(mode*)(list - sizeof(mode))))
