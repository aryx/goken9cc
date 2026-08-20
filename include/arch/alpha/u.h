/* claude: uintptr/intptr/usize deliberately `long long`/`unsigned long
 * long` (vlong-width, 8 bytes), NOT `long`/`unsigned long` -- this
 * compiler's `long` is only 4 bytes even here, on a genuine 64-bit
 * arch (confirmed via a direct zc -S probe: sizeof(long)==4 while
 * sizeof(void*)==8), the exact same trap arm64/amd64/riscv64's own
 * u.h already document and fix. The original import had uintptr/
 * intptr/usize as plain `long`/`unsigned long`, which would silently
 * truncate any pointer cast through them to 32 bits -- e.g. every
 * rt0.s store into port/mainargs.c's `intptr _mainargc`, or
 * include/core/exn.h's `struct Jmpbuf { uintptr sp, pc; }` (see
 * lib_core/libc/arch/alpha/setjmp.s), which setjmp.s's own 8-byte
 * MOVQ stores would then overflow past. See
 * docs/claude_notes/notes_arch_alpha.txt for the full story.
 */
/* claude: s8/u8/s16/u16/s32/u32/s64/u64 -- required by
 * include/core/types.h ("typedef s8 int8;" etc, line 21 on), which
 * every other arch's u.h (arm64, riscv64, ...) already defines these
 * for. Missing from the original import entirely (it only had the
 * older u8int/u32int/... aliases below, not these) -- every port/*.c
 * and fmt/*.c file failed to parse with "not a function ... last name:
 * int8" until these were added, since int8 itself never resolved.
 */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;

/* claude: nil NOT defined here (unlike the original import) -- it's
 * already in include/core/macros.h, included generically for every
 * arch; a second #define here (even an identical one) is a hard error
 * for this compiler ("macro redefined: nil"), not a warning. */
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned int	uint;
typedef   signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef long long	intptr;
typedef unsigned long long uintptr;
typedef unsigned long long usize;
typedef	uint		Rune;
typedef 	union FPdbleword FPdbleword;
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int	u32int;
typedef unsigned long long u64int;

/* FCR */
#define	FPINEX	(1<<30)
#define	FPOVFL	(1<<19)
#define	FPUNFL	((1<<29)|(1<<28))
#define	FPZDIV	(1<<18)
#define	FPINVAL	(1<<17)

#define	FPRNR	(2<<26)
#define	FPRZ		(0<<26)
#define	FPRPINF	(3<<26)
#define	FPRNINF	(1<<26)
#define	FPRMASK	(3<<26)

#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<24)
#define	FPAUNFL	(1<<23)
#define	FPAOVFL	(1<<22)
#define	FPAZDIV	(1<<21)
#define	FPAINVAL	(1<<20)
union FPdbleword
{
	double	x;
	struct {	/* little endian */
		ulong lo;
		ulong hi;
	};
};

/* stdarg */
typedef	char*	va_list;
#define va_start(list, start) list =\
	(sizeof(start) < 4?\
		(char*)((int*)&(start)+1):\
		(char*)(&(start)+1))
#define va_end(list)\
	USED(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((list += 4), (mode*)list)[-4]:\
	(sizeof(mode) == 2)?\
		((list += 4), (mode*)list)[-2]:\
	sizeof(mode)>4?\
		((mode*)(list = (char*)((uintptr)(list+7) & ~7) + sizeof(mode)))[-1]:\
		((list += sizeof(mode)), (mode*)list)[-1])
