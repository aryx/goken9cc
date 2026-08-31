/* claude: nil NOT defined here -- it's already in
 * include/core/macros.h, included generically for every arch; a
 * second #define here (even an identical one) is a hard error for
 * this compiler ("macro redefined: nil"). Same fix already applied to
 * power/arm64/riscv64/alpha's own u.h. */

/* claude: s8/u8/s16/u16/s32/u32/s64/u64 -- required by
 * include/core/types.h ("typedef s8 int8;" etc), which every other
 * arch's u.h already defines these for. Missing from the original
 * import entirely, same gap already found and fixed on power's u.h. */
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
 * here, NOT vlong -- unlike alpha/arm64/riscv64/amd64's own u.h (see
 * alpha's own comment on this exact typedef), sparc is a genuine
 * 32-bit target: this compiler's `long` is always 4 bytes, kc's own
 * pointers are 4 bytes, so the two never diverge here the way they do
 * on a 64-bit arch with a 4-byte `long`. Matches every other 32-bit
 * arch's own u.h (386/arm/mips/power). */
typedef long		intptr;
typedef unsigned long	uintptr;
typedef unsigned long	usize;
typedef	uint		Rune;
typedef 	union FPdbleword FPdbleword;
/* claude: jmp_buf NOT defined here -- include/core/exn.h already
 * provides the real one (Jmpbuf{sp,pc}), shared across every arch;
 * this file's own jmp_buf[2]/JMPBUFSP/PC/DPC were leftovers from the
 * original import and conflict with it. Same fix already applied to
 * power/alpha's own u.h. */
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int	u32int;
typedef unsigned long long u64int;

/* FCR */
#define	FPINEX	(1<<23)
#define	FPOVFL	(1<<26)
#define	FPUNFL	(1<<25)
#define	FPZDIV	(1<<24)
#define	FPRNR	(0<<30)
#define	FPRZ	(1<<30)
#define	FPINVAL	(1<<27)
#define	FPRPINF	(2<<30)
#define	FPRNINF	(3<<30)
#define	FPRMASK	(3<<30)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<5)
#define	FPAZDIV	(1<<6)
#define	FPAUNFL	(1<<7)
#define	FPAOVFL	(1<<8)
#define	FPAINVAL	(1<<9)
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
