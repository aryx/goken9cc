/* claude: nil NOT defined here -- it's already in
 * include/core/macros.h, included generically for every arch; a
 * second #define here (even an identical one) is a hard error for
 * this compiler ("macro redefined: nil"). Same fix already applied to
 * power/sparc/arm64/riscv64/alpha's own u.h. */

/* claude: s8/u8/s16/u16/s32/u32/s64/u64 -- required by
 * include/core/types.h ("typedef s8 int8;" etc), which every other
 * arch's u.h already defines these for. Missing from the original
 * import entirely, same gap already found and fixed on power/sparc's
 * own u.h. */
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

typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
/* claude: intptr/uintptr deliberately plain `long`/`unsigned long`
 * here, NOT vlong -- see sparc/u.h's own comment on this exact
 * typedef: m68k is a genuine 32-bit target, this compiler's `long` is
 * always 4 bytes, 2c's own pointers are 4 bytes, so the two never
 * diverge here the way they do on a 64-bit arch with a 4-byte `long`
 * (alpha/arm64/riscv64/amd64's own u.h). Matches every other 32-bit
 * arch's own u.h (386/arm/mips/power/sparc). */
typedef long		intptr;
typedef unsigned long	uintptr;
typedef unsigned long	usize;
/* claude: uint, not ushort -- include/utf/utf.h's own `typedef uint
 * Rune;` is what every other arch's u.h already agrees with (a Unicode
 * code point needs more than 16 bits); this file's own ushort was a
 * leftover from the original import that conflicted with it outright
 * ("external redeclaration of: Rune"). */
typedef	uint		Rune;
typedef 	union FPdbleword FPdbleword;
/* claude: jmp_buf NOT defined here -- include/core/exn.h already
 * provides the real one (Jmpbuf{sp,pc}), shared across every arch;
 * this file's own jmp_buf[2]/JMPBUFSP/PC/DPC were leftovers from the
 * original import and conflict with it. Same fix already applied to
 * power/sparc/alpha's own u.h. */
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int	u32int;
typedef unsigned long long u64int;


/* FCR */
#define	FPINEX	(3<<8)
#define	FPOVFL	(1<<12)
#define	FPUNFL	(1<<11)
#define	FPZDIV	(1<<10)
#define	FPRNR	(0<<4)
#define	FPRZ	(1<<4)
#define	FPINVAL	(3<<13)
#define	FPRPINF	(3<<4)
#define	FPRNINF	(2<<4)
#define	FPRMASK	(3<<4)
#define	FPPEXT	(0<<6)
#define	FPPSGL	(1<<6)
#define	FPPDBL	(2<<6)
#define	FPPMASK	(3<<6)
/* FSR */
#define	FPAINEX	FPINEX
#define	FPAOVFL	FPOVFL
#define	FPAUNFL	FPUNFL
#define	FPAZDIV	FPZDIV
#define	FPAINVAL	FPINVAL
union FPdbleword
{
	double	x;
	struct {	/* big endian */
		ulong hi;
		ulong lo;
	};
};

typedef	char*	va_list;
#define va_start(list, start) list =\
	(sizeof(start) < 4?\
		(char*)((int*)&(start)+1):\
		(char*)(&(start)+1))
#define va_end(list)\
	USED(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((list += 4), (mode*)list)[-1]:\
	(sizeof(mode) == 2)?\
		((list += 4), (mode*)list)[-1]:\
		((list += sizeof(mode)), (mode*)list)[-1])
