// code taken from GO/src/pkg/runtime/

/*
 * basic types
 */
typedef	signed char            int8;
typedef	unsigned char          uint8;
typedef	signed short           int16;
typedef	unsigned short         uint16;

// Note that this is also correct on 64-bit archs with Linux
// and macOS where sizeof(int) is also 4.
typedef	signed int             int32;
typedef	unsigned int           uint32;

#ifndef NO_64
typedef	signed long long int   int64;
typedef	unsigned long long int uint64;
#endif

#ifndef NO_FLOAT
typedef	float			float32;
typedef	double			float64;
#endif

#ifdef _64BIT
typedef	uint64		uintptr;
typedef	int64		intptr;
#else
typedef	uint32		uintptr;
typedef int32		intptr;
#endif

/*
 * get rid of C types
 * the / / / forces a syntax error immediately,
 * which will show "last name: XXunsigned".
 */
#define	unsigned		XXunsigned / / /
#define	signed			XXsigned / / /
//#define	char			XXchar / / /
#define	short			XXshort / / /
//#define	int			XXint / / /
#define	long			XXlong / / /
#define	float			XXfloat / / /
#define	double			XXdouble / / /

/*
 * defined types
 */
typedef	uint8			bool;
typedef	uint8			byte;

enum
{
	true	= 1,
	false	= 0,
};

/*
 * defined macros
 *    you need super-goru privilege
 *    to add this list.
 */
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	nil		((void*)0)


// extern decls
extern void	panic(int32);
extern int32 findnull(byte*);

// float.c
#ifndef NO_64
extern bool	isInf(float64 f, int32 sign);
extern bool	isNaN(float64 f);
#endif

// print.c
extern void printf(char *s, ...);
//void ·printpc(void *p);

//for printf typechecker?
//#pragma	varargck	argpos	printf	1
//#pragma	varargck	type	"d"	int32
//#pragma	varargck	type	"d"	uint32
//#pragma	varargck	type	"D"	int64
//#pragma	varargck	type	"D"	uint64
//#pragma	varargck	type	"x"	int32
//#pragma	varargck	type	"x"	uint32
//#pragma	varargck	type	"X"	int64
//#pragma	varargck	type	"X"	uint64
//#pragma	varargck	type	"p"	void*
//#pragma	varargck	type	"p"	uintptr
//#pragma	varargck	type	"s"	int8*
//#pragma	varargck	type	"s"	uint8*
//#pragma	varargck	type	"S"	String

// linux.s
extern void write(uint32 fd, char* buf, /*size_t*/ int count);
extern void exit(uint32);
void*	·getcallerpc(void*);
