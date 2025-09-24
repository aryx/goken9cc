/*
 * basic types
 */
typedef	signed char            int8;
typedef	unsigned char          uint8;
typedef	signed short           int16;
typedef	unsigned short         uint16;

//TODO??? this is correct on 64 archs?????
typedef	signed int             int32;
typedef	unsigned int           uint32;
typedef	signed long long int   int64;
typedef	unsigned long long int uint64;

//TODO??? again correct on 64 archs???
typedef	float			float32;
typedef	double			float64;

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
extern bool	isInf(float64 f, int32 sign);
extern bool	isNaN(float64 f);

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
extern void write(/*unsigned int*/int fd, char* buf, /*size_t*/ int count);
extern void exit(/*unsigned int*/int);
void*	·getcallerpc(void*);
