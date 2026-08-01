/* riscv64 (rv64) base types for goken's own toolchain (jc/ja/jl)
 *
 * Typedefs adapted from ~/xxx/miller-riscv/ROOT/riscv64/include/u.h
 * (Richard Miller's Plan9 riscv64 port -- see plan9front.txt's "riscv"
 * section)
 * va_start/va_arg verified
 * (not just copied) against real jc -S output and actual qemu-riscv64
 * execution, same methodology as riscv/u.h.
 *
 * CRITICAL and easy to get wrong: like every other Plan9 C compiler in
 * this tree, jc's `long` is 4 bytes even here, on a genuine 64-bit
 * arch (confirmed: sizeof(long)==4 via a direct probe) -- but pointers
 * are 8 bytes. uintptr/intptr below are deliberately `unsigned long
 * long`/`long long` (vlong-width), NOT `unsigned long`/`long` the way
 * arm64's/amd64's u.h (correctly) use, because those two *do* have an
 * 8-byte `long`. Using `ulong` here would silently truncate any
 * pointer cast through it to 32 bits -- this is exactly what happened
 * while deriving this file's own va_arg alignment logic (a `(uintptr)
 * list` cast through a wrongly-`ulong`-typed uintptr corrupted the
 * pointer and segfaulted under qemu; switching to `unsigned long long`
 * fixed it immediately, no other change needed). The same trap applies
 * to lib_core/libc/syscall/os/linux/syscall_linux_riscv64.h's
 * _syscall6 declaration -- see that file's own comment.
 */


typedef signed char s8int;
typedef unsigned char u8int;
typedef signed short s16int;
typedef unsigned short u16int;
typedef signed int s32int;
typedef unsigned int u32int;
typedef signed long long s64int;
typedef unsigned long long u64int;

typedef float float32;
typedef double float64;

// see this file's own header comment: deliberately vlong-width, not
// `unsigned long`/`long` (only 4 bytes here, unlike arm64/amd64).
typedef unsigned long long uintptr;
typedef long long intptr;

// bit-level double access for port/frexp.c (frexp/ldexp/modf) -- rv64
// is little-endian; ulong is still only 4 bytes here (see header
// comment), matching uint32, so no size-mismatch special-casing needed
// (unlike arm64/amd64, whose 8-byte ulong would double this union).
union FPdbleword {
	double x;
	struct {	/* little endian */
		unsigned long lo;
		unsigned long hi;
	};
};
typedef union FPdbleword FPdbleword;

/* Plan9-style va_list for riscv64 (rv64).
 *
 * Same rule as riscv/u.h (rv32): natural sizeof()-packing, with any
 * 8-byte-or-wider type (double, vlong, and on this arch also any
 * *pointer*, since uintptr is 8 bytes here unlike rv32) forcing the
 * list pointer to round up to 8-byte alignment first. Confirmed via
 * actual qemu-riscv64 execution of a real 3-argument (int, long,
 * double) va_arg sequence, not just -S inspection -- see this file's
 * own header comment for why a naive `ulong`-based version of this
 * exact macro segfaulted before the uintptr fix.
 */

typedef char* va_list;

#define va_start(list, start) \
	(list = (sizeof(start) < 4 ? (char*)((int*)&(start) + 1) : (char*)(&(start) + 1)))

#define va_end(list)

#define va_arg(list, mode) \
	((sizeof(mode) < 8) ? \
		((list += 4), *(mode*)(list - 4)) : \
		(list = (char*)(((uintptr)list + 7) & ~7) + sizeof(mode), *(mode*)(list - sizeof(mode))))
