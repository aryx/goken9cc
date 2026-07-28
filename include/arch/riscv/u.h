/* riscv (rv32) base types for goken's own toolchain (ic/ia/il).
 *
 * Freestanding: no Unix/host headers (lib_core/ code may only depend on
 * headers under include/, never on the host's libc -- that's lib9/'s job).
 *
 * Typedefs adapted from ~/xxx/miller-riscv/ROOT/riscv/include/u.h
 * (Richard Miller's Plan9 riscv port -- see plan9front.txt's "riscv"
 * section) -- principia and 9front have no riscv port to draw from for
 * this arch, unlike arm64/mips. Trimmed relative to that fuller
 * version (no jmp_buf/mpdigit/FCR-FSR flags yet, matching the same
 * "add later if lib_core/libc actually needs it" policy as arm64's/
 * mips's u.h) -- but va_start/va_arg were NOT copied as-is: verified
 * (and, for one case, corrected) against real ic -S output and actual
 * qemu-riscv execution below, since goken's own ic is a different,
 * independently-implemented compiler, not the same binary as
 * miller-riscv's.
 */

#define nil ((void*)0)

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef long long vlong;
typedef unsigned long long uvlong;

typedef signed char s8int;
typedef unsigned char u8int;
typedef signed short s16int;
typedef unsigned short u16int;
typedef signed int s32int;
typedef unsigned int u32int;
typedef signed long long s64int;
typedef unsigned long long u64int;

typedef s8int int8;
typedef u8int uint8;
typedef s16int int16;
typedef u16int uint16;
typedef s32int int32;
typedef u32int uint32;
typedef s64int int64;
typedef u64int uint64;

typedef float float32;
typedef double float64;

typedef unsigned long uintptr;
typedef long intptr;
typedef intptr ptrdiff;

// fmt/fmtfd.c uses the raw C99 stdint.h name in one spot rather than
// the Plan9-style alias above; can't #include <stdint.h> (Unix header),
// so just alias it here instead of editing that ported file.
typedef uintptr uintptr_t;

#ifndef __bool_true_false_are_defined
typedef uint8 bool;
enum {
	false = 0,
	true = 1,
};
#define __bool_true_false_are_defined 1
#endif

typedef uint8 byte;

// bit-level double access for port/frexp.c (frexp/ldexp/modf) -- rv32
// is little-endian (like arm64/amd64/386, unlike big-endian mips), and
// ulong is 4 bytes here (matches uint32, no arm64-style size mismatch
// to worry about).
union FPdbleword {
	double x;
	struct {	/* little endian */
		ulong lo;
		ulong hi;
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
