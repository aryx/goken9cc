/*
 * basic types
 */
typedef	signed char		int8;
typedef	unsigned char		uint8;
typedef	signed short		int16;
typedef	unsigned short		uint16;
typedef	signed int		int32;
typedef	unsigned int		uint32;
typedef	signed long long int	int64;
typedef	unsigned long long int	uint64;
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
 * defined types
 */
typedef	uint8			bool;
typedef	uint8			byte;

/*
 * defined macros
 *    you need super-goru privilege
 *    to add this list.
 */
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	nil		((void*)0)

// extern decls
extern void	panic(int32);
extern int32	findnull(byte*);

// float.c
extern bool	isInf(float64 f, int32 sign);
extern bool	isNaN(float64 f);

// print.c
extern void printf(int8 *s, ...);
//void ·printpc(void *p);

// linux.s
extern void write(int32 fd, char* buf, int32 n);
extern void exit(int);
void*	·getcallerpc(void*);
