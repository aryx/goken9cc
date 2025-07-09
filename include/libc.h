/*
Derived from Inferno include/kern.h and
Plan 9 from User Space include/libc.h

http://code.google.com/p/inferno-os/source/browse/include/kern.h
http://code.swtch.com/plan9port/src/tip/include/libc.h

Copyright © 1994-1999 Lucent Technologies Inc.  All rights reserved.
Revisions Copyright © 2000-2007 Vita Nuova Holdings Limited (www.vitanuova.com).  All rights reserved.
Portions Copyright © 2001-2007 Russ Cox.  All rights reserved.
Portions Copyright © 2009 The Go Authors.  All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef _LIBC_H_
#define _LIBC_H_ 1

//******************************************************************************
// Prelude
//******************************************************************************
/*
 * Lib9 is miscellany from the Plan 9 C library that doesn't
 * fit into libutf or into libfmt, but is still missing from traditional
 * Unix C libraries.
 */

#include <utf.h>
#include <fmt.h>

/*
 * Begin trimmed down usual libc.h
 */

//******************************************************************************
// Macros
//******************************************************************************

#ifndef nil
#define	nil	((void*)0)
#endif
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

#ifndef offsetof
#define offsetof(s, m)	(ulong)(&(((s*)0)->m))
#endif

//******************************************************************************
// Pad's stuff (also in principia/include/ALL/libc.h)
//******************************************************************************

// Those types are needed to compile src/cmd/mk which
// comes from pad's principia which use a few extra C types
// (I like types).
#if __STDC_VERSION__ < 202311L  // before C23
#ifndef __bool_true_false_are_defined
typedef	uint8			bool;
enum _bool
{
	true	= 1,
	false	= 0,
};
#define __bool_true_false_are_defined 1
#endif
#endif

typedef	uint8			byte;

#define STDIN 0
#define STDOUT 1
#define STDERR 2
typedef int fdt; // file descriptor type

// in principia/include/ALL/syscall.h
enum Seek_cursor {
    SEEK__START = 0,
    SEEK__CUR = 1,
    SEEK__END = 2,
};

#define OK_0 0
#define OK_1 1
#define ERROR_0 0
#define ERROR_1 1
#define ERROR_NEG1 (-1)
typedef int error0; // 0 is the error value
typedef int error1; // 1 is the error value
typedef int errorneg1; // -1 is the error value
typedef int errorn; // 1 or more means error

//******************************************************************************
// Bytes
//******************************************************************************

// memset(), memcpy(), memcmp(), memchr() in string.h
// memmove(), memccpy()  in string.h

//******************************************************************************
// Strings
//******************************************************************************

// strcpy(), strcmp(), strchr(), strrchr() in string.h
// strlen(), strcat(), strdup(), strstr() in string.h

// tolower(), toupper() ???

extern	char* strecpy(char*, char*, char*);
// strncat(), strncpy(), strncmp() ??

// break a string into fields
extern  int tokenize(char*, char**, int);
//??
extern	int getfields(char*, char**, int, int, char*);
//??
extern	int gettokens(char *, char **, int, char *);

// strtok() ?

//******************************************************************************
// Conversions
//******************************************************************************
extern	int	p9atoi(char*);
extern	long	p9atol(char*);
extern	vlong	p9atoll(char*);

extern	double	fmtcharstod(int(*)(void*), void*);

//******************************************************************************
// Math
//******************************************************************************

// abs() defined in stdlib.h

extern  int isInf(double, int);
extern	double	frexp(double, int*);
extern	double	p9pow10(int);

//******************************************************************************
// File
//******************************************************************************

// enum Open_flag, open parameter
#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */

// advanced stuff (no O_APPEND, O_CREATE, O_NONBLOCK as in Unix though)
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	ORCLOSE	64	/* or'ed in, remove on close */
#define	ODIRECT	128	/* or'ed in, direct access */
#define	OEXCL	0x1000	/* or'ed in, exclusive use (create only) */

#define	OAPPEND	0x4000	/* or'ed in, append only */

extern	fdt	p9open(char*, int);
extern	int	close(fdt);

// read(), write() in unistd.h
extern	long	readn(fdt, void*, long);
extern	vlong	p9seek(fdt, vlong, int);

extern	int	p9dup(fdt, fdt);

// missing pread(), pwrite() for now and preadv/pwritev, readv/writev

//******************************************************************************
// Directory
//******************************************************************************

#define	STATMAX	65535U	/* max length of machine-independent stat structure */
#define	DIRMAX	(sizeof(Dir)+STATMAX)	/* max length of Dir structure */

// enum Access_flag
#define	AEXIST	0	/* accessible: exists */
#define	AEXEC	1	/* execute access */
#define	AWRITE	2	/* write access */
#define	AREAD	4	/* read access */

/* bits in Qid.type */
#define QTFILE		0x00		/* type bits for plain file */
#define QTDIR		0x80		/* type bit for directories */
// advanced stuff
#define QTAPPEND	0x40		/* type bit for append only files */
#define QTEXCL		0x20		/* type bit for exclusive use files */
#define QTMOUNT		0x10		/* type bit for mounted channel */
#define QTAUTH		0x08		/* type bit for authentication file */
#define QTTMP		0x04		/* type bit for non-backed-up file */
#define QTSYMLINK	0x02		/* type bit for symbolic link */

// Qid as in uniQue id
typedef
struct Qid
{
	uvlong	path;
	ulong	vers;
	uchar	type;
} Qid;

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMREAD		0x4		/* mode bit for read permission */
#define DMWRITE		0x2		/* mode bit for write permission */
#define DMEXEC		0x1		/* mode bit for execute permission */
// advanced stuff
#define DMAPPEND	0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMAUTH		0x08000000	/* mode bit for authentication file */
#define DMTMP		0x04000000	/* mode bit for non-backed-up file */
// unix extensions
#define DMSYMLINK	0x02000000	/* mode bit for symbolic link (Unix, 9P2000.u) */
#define DMDEVICE	0x00800000	/* mode bit for device file (Unix, 9P2000.u) */
#define DMNAMEDPIPE	0x00200000	/* mode bit for named pipe (Unix, 9P2000.u) */
#define DMSOCKET	0x00100000	/* mode bit for socket (Unix, 9P2000.u) */
#define DMSETUID	0x00080000	/* mode bit for setuid (Unix, 9P2000.u) */
#define DMSETGID	0x00040000	/* mode bit for setgid (Unix, 9P2000.u) */


typedef
struct Dir {
	/* system-modified data */
	ushort	type;	/* server type */
	uint	dev;	/* server subtype */
	/* file data */
	Qid	qid;	/* unique id from server */
	ulong	mode;	/* permissions */
	ulong	atime;	/* last read time */
	ulong	mtime;	/* last write time */
	vlong	length;	/* file length */
	char	*name;	/* last element of path */
	char	*uid;	/* owner name */
	char	*gid;	/* group name */
	char	*muid;	/* last modifier name */

	/* 9P2000.u extensions */
	uint	uidnum;		/* numeric uid */
	uint	gidnum;		/* numeric gid */
	uint	muidnum;	/* numeric muid */
	char	*ext;		/* extended info */
} Dir;

extern	int	p9create(char*, int, ulong);
// remove() also in stdio.h
extern	int	remove(const char*);

// missing fstat(), stat(), fwstat(), wstat() compared to plan9 libc
// missinc access()

extern	Dir*	dirstat(char*);
extern	Dir*	dirfstat(int);
extern	int	dirwstat(char*, Dir*);
extern	int	dirfwstat(int, Dir*);

extern	void	nulldir(Dir*);
extern	long	dirreadall(int, Dir**); // ?? defined where?

//******************************************************************************
// Path
//******************************************************************************

extern	int	fd2path(int, char*, int);

extern	char*	cleanname(char*);
extern	char*	unsharp(char*);

//PAD: why was in libc.h?? it's part of rc actually
//extern	char*	searchpath(char*);

//******************************************************************************
// Namespaces
//******************************************************************************
// enum Namespace_flag, mount/bind parameter
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */

#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MCACHE	0x0010	/* cache some data */

#define	MMASK	0x0017	/* all bits on */

// bitset<Namespace_flag>
#define	MORDER	0x0003	/* mask for bits defining order of mounting */

#ifdef RFMEM	/* FreeBSD, OpenBSD */
#undef RFFDG
#undef RFNOTEG
#undef RFPROC
#undef RFMEM
#undef RFNOWAIT
#undef RFCFDG
#undef RFNAMEG
#undef RFENVG
#undef RFCENVG
#undef RFCFDG
#undef RFCNAMEG
#endif

// Rfork_flags
enum
{
	RFNAMEG		= (1<<0),
	RFENVG		= (1<<1),
	RFFDG		= (1<<2),
	RFNOTEG		= (1<<3),
	RFPROC		= (1<<4),
	RFMEM		= (1<<5),
	RFNOWAIT	= (1<<6),
	RFCNAMEG	= (1<<10),
	RFCENVG		= (1<<11),
	RFCFDG		= (1<<12)
/*	RFREND		= (1<<13), */
/*	RFNOMNT		= (1<<14) */
};

// missing bind(), mount(), unmount() compared to plan9 libc

//******************************************************************************
// Tmp files
//******************************************************************************
extern	char*	mktemp(char*);
extern	int	opentemp(char*);

//******************************************************************************
// Process
//******************************************************************************
extern	int	exitcode(char*);
extern	void	exits(char*);
extern	void	_exits(char*);

extern	char*	p9getenv(char*);
extern	int	p9putenv(char*, char*);


// missing getpid(), getppid() compared to plan9 libc

extern	char*	p9getwd(char*, int);
extern	int	p9chdir(char*);

extern	int	p9exec(char*, char*[]);
extern	int	p9execl(char*, ...);

// fork() in unistd.h
extern	int	p9rfork(int);

extern	int	await(char*, int);
extern	int	awaitfor(int, char*, int);
extern	int	awaitnohang(char*, int);

typedef
struct Waitmsg
{
	int pid;	/* of loved one */
	ulong time[3];	/* of loved one & descendants */
	char	*msg;
} Waitmsg;

extern	Waitmsg*	p9wait(void);
extern	Waitmsg*	p9waitfor(int);
extern	Waitmsg*	waitnohang(void);
extern	int	p9waitpid(void);

//******************************************************************************
// Memory
//******************************************************************************

// no sbrk() compared to plan9 libc
// malloc() and free() in stdlib.h
// mallocz ?? 

//******************************************************************************
// Time
//******************************************************************************


// time() defined in time.h but not included in u.h

extern  double  p9cputime(void);
// nsec() ? 

// gmtime(), localtime() ??

extern	long	p9alarm(ulong);
extern	int	p9sleep(long);

//******************************************************************************
// Random
//******************************************************************************
// rand(), srand(), frand(), nrand(), truerand() ??
// lrand(), lnrand(), ntruerand() ??

//******************************************************************************
// Concurrency
//******************************************************************************

// ainc(), adec() ??
// cas32(), casp(), casl() ??

// Lock? _tas(), lock(), unlock(), canlock() ?
// QLock? RWLock?

extern	void	p9longjmp(p9jmp_buf, int);
extern	void	p9notejmp(void*, p9jmp_buf, int);
#define p9setjmp(b)	sigsetjmp((void*)(b), 1)

//******************************************************************************
// IPC
//******************************************************************************

// pipe() in unistd.h

/* Segattch */
#define	SG_RONLY	0040	/* read only */
#define	SG_CEXEC	0100	/* detach on exec */

// no segattach(), segbrk(), segdetach(), setflush(), segfree()

#define	NCONT	0	/* continue after note */
#define	NDFaLT	1	/* terminate after note */
#define	NSAVE	2	/* clear note but hold state */
#define	NRSTR	3	/* restore saved state */

extern	int	postnote(int, int, char *);
extern	int	noted(int);
extern	int	notify(void(*)(void*, char*));
extern	int	noteenable(char*);
extern	int	notedisable(char*);
extern	int	notifyon(char*);
extern	int	notifyoff(char*);

// Rendez type?
extern	ulong	rendezvous(ulong, ulong);
// rsleep()? rwakeup()?

// no semacquire(), semrelease(), tsemacquire()

//******************************************************************************
// Network
//******************************************************************************
// accept(), announce(), dial(), listen() ??

//******************************************************************************
// Security
//******************************************************************************

// no fauth(), no fversion()

// ??
extern	char*	getuser(void);

//******************************************************************************
// GOXXX special vars
//******************************************************************************

extern	char*	getgoos(void);
extern	char*	getgoarch(void);

extern	char*	getgoroot(void);
extern	char*	getgoversion(void);

//******************************************************************************
// Error management
//******************************************************************************
#define	ERRMAX	128	/* max length of error string */

extern	void	perror(const char*);

extern	int	errstr(char*, uint);

extern	void	rerrstr(char*, uint);
extern	void	werrstr(char*, ...);

extern	void	sysfatal(char*, ...);
extern	void	abort(void);

//******************************************************************************
// Misc
//******************************************************************************

/*
 * one-of-a-kind
 */
enum
{
	PNPROC		= 1,
	PNGROUP		= 2
};


//??
extern	char*	sysname(void);

//??
extern	char*	getns(void);

//??
extern	char*	get9root(void);

#ifndef NOPLAN9DEFINES

#define cputime     p9cputime

#define atoi		p9atoi
#define atol		p9atol
#define atoll		p9atoll
#define strtod		fmtstrtod
#define charstod	fmtcharstod

#define getenv		p9getenv
#define putenv		p9putenv

#define	getwd		p9getwd

#define	longjmp		p9longjmp
#undef  setjmp
#define setjmp		p9setjmp
#define notejmp		p9notejmp
#define jmp_buf		p9jmp_buf

#define pow10		p9pow10

#undef open
#define open		p9open
#define create		p9create
#define	seek		p9seek
#define	dup		p9dup

#define	exec		p9exec
#define	execl	        p9execl
#define rfork		p9rfork
#define wait		p9wait
#define waitpid	        p9waitpid
#define	waitfor		p9waitfor

#define sleep		p9sleep
#define alarm		p9alarm

// not declared in this file
#define main	p9main

#endif

//******************************************************************************
// Windows
//******************************************************************************

#ifdef __MINGW32__
struct timespec {
	int tv_sec;
	long tv_nsec;
};
extern int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
extern int fork(void);
extern int pread(int fd, void *buf, int n, int off);
extern int pwrite(int fd, void *buf, int n, int off);
#define execv(prog, argv) execv(prog, (const char* const*)(argv))
#define execvp(prog, argv) execvp(prog, (const char**)(argv))
#define lseek(fd, n, base) _lseeki64(fd, n, base)
#define mkdir(path, perm) mkdir(path)
#define pipe(fd) _pipe(fd, 512, O_BINARY)
#else
#define O_BINARY 0
#endif

//******************************************************************************
// SET/USED marking
//******************************************************************************

/* compiler directives on plan 9 */
#define	SET(x)	((x)=0)
#define	USED(x)	if(x){}else{}
#ifdef __GNUC__
#	if __GNUC__ >= 3
#		undef USED
#		define USED(x) ((void)(x))
#	endif
#endif

//******************************************************************************
// CLI macros
//******************************************************************************
// use??
extern char	*argv0;

// ugly hack for macOS I think
extern void __fixargv0(void);

#define	ARGBEGIN	for((argv0?0:(argv0=(__fixargv0(),*argv))),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				Rune _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while(*_args && (_args += chartorune(&_argc, _args)))\
				switch(_argc)

#define	ARGEND		SET(_argt);USED(_argt);USED(_argc);USED(_args);}USED(argv);USED(argc);

#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))

#define	EARGF(x)	(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): ((x), abort(), (char*)0)))

#define	ARGC()		_argc

//******************************************************************************
// Postlude
//******************************************************************************

#endif	/* _LIB9_H_ */
