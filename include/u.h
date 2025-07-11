/*
Plan 9 from User Space include/u.h
http://code.swtch.com/plan9port/src/tip/include/u.h

Copyright 2001-2007 Russ Cox.  All Rights Reserved.

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
#ifndef _U_H_
#define _U_H_ 1

//******************************************************************************
// Prelude
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************

#define __BSD_VISIBLE 1 /* FreeBSD 5.x */
#define _BSD_SOURCE 1
#define _NETBSD_SOURCE 1	/* NetBSD */
#define _SVID_SOURCE 1

#if !defined(__APPLE__) && !defined(__OpenBSD__)
#	define _XOPEN_SOURCE 1000
#	define _XOPEN_SOURCE_EXTENDED 1
#endif

#if defined(__FreeBSD__)
#	include <sys/cdefs.h>
	/* for strtoll */
#	undef __ISO_C_VISIBLE
#	define __ISO_C_VISIBLE 1999
#	undef __LONG_LONG_SUPPORTED
#	define __LONG_LONG_SUPPORTED
#endif

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

//******************************************************************************
// Standard includes (=~ libc.h)
//******************************************************************************

#include <inttypes.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>	/* for tolower */
#include <signal.h>

//******************************************************************************
// OS-specific crap
//******************************************************************************

#define _NEEDUCHAR 1
#define _NEEDUSHORT 1
#define _NEEDUINT 1
#define _NEEDULONG 1

#ifdef __MINGW32__
typedef jmp_buf sigjmp_buf;
#endif

typedef long p9jmp_buf[sizeof(sigjmp_buf)/sizeof(long)];

#if defined(__linux__)
#	include <sys/types.h>
#	if defined(__Linux26__)
#		include <pthread.h>
#		define PLAN9PORT_USING_PTHREADS 1
#	endif
#	if defined(__USE_MISC)
#		undef _NEEDUSHORT
#		undef _NEEDUINT
#		undef _NEEDULONG
#	endif
#elif defined(__FreeBSD__)
#	include <sys/types.h>
#	include <osreldate.h>
#	if __FreeBSD_version >= 500000
#		define PLAN9PORT_USING_PTHREADS 1
#		include <pthread.h>
#	endif
#	if !defined(_POSIX_SOURCE)
#		undef _NEEDUSHORT
#		undef _NEEDUINT
#	endif
#elif defined(__APPLE__)
#	include <sys/types.h>
#	include <pthread.h>
#	define PLAN9PORT_USING_PTHREADS 1
#	if __GNUC__ < 4
#		undef _NEEDUSHORT
#		undef _NEEDUINT
#	endif
#	undef _ANSI_SOURCE
#	undef _POSIX_C_SOURCE
#	undef _XOPEN_SOURCE
#	if !defined(NSIG)
#		define NSIG 32
#	endif
#	define _NEEDLL 1
#elif defined(__NetBSD__)
#	include <sched.h>
#	include <sys/types.h>
#	undef _NEEDUSHORT
#	undef _NEEDUINT
#	undef _NEEDULONG
#elif defined(__OpenBSD__)
#	include <sys/types.h>
#	undef _NEEDUSHORT
#	undef _NEEDUINT
#	undef _NEEDULONG
#elif defined(__MINGW32__)
#else
	/* No idea what system this is -- try some defaults */
#	include <pthread.h>
#	define PLAN9PORT_USING_PTHREADS 1
#endif

#ifndef O_DIRECT
#define O_DIRECT 0
#endif

//******************************************************************************
// uxxx
//******************************************************************************

typedef signed char schar;

#ifdef _NEEDUCHAR
	typedef unsigned char uchar;
#endif
#ifdef _NEEDUSHORT
	typedef unsigned short ushort;
#endif
#ifdef _NEEDUINT
	typedef unsigned int uint;
#endif
#ifdef _NEEDULONG
	typedef unsigned long ulong;
#endif

typedef unsigned long long uvlong;
typedef long long vlong;

typedef uint64_t u64int;
typedef int64_t s64int;
typedef uint8_t u8int;
typedef int8_t s8int;
typedef uint16_t u16int;
typedef int16_t s16int;
typedef uintptr_t uintptr;
typedef intptr_t intptr;
typedef uint32_t u32int;
typedef int32_t s32int;

typedef s8int int8;
typedef u8int uint8;
typedef s16int int16;
typedef u16int uint16;
typedef s32int int32;
typedef u32int uint32;
typedef s64int int64;
typedef u64int uint64;

#undef _NEEDUCHAR
#undef _NEEDUSHORT
#undef _NEEDUINT
#undef _NEEDULONG

//******************************************************************************
// Pad's stuff (also in principia/include/ALL/libc.h)
//******************************************************************************

// Those types are needed to compile src/cmd/{mk,rc} which
// comes from pad's principia which use a few extra C types
// (I like types, and I especially don't like abusing ints for everything)

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

enum _ord {
  ORD__EQ = 0,
  ORD__INF = -1,
  ORD__SUP = 1,
};
typedef int ord;

#define STDIN 0
#define STDOUT 1
#define STDERR 2
typedef int fdt; // file descriptor type

// in principia/include/ALL/syscall.h
enum _seek_cursor {
    SEEK__START = 0,
    SEEK__CUR = 1,
    SEEK__END = 2,
};

typedef int pidt; // pid type

#define OK_0 0
#define OK_1 1
#define ERROR_0 0
#define ERROR_1 1
#define ERROR_NEG1 (-1)
typedef int error0; // 0 is the error value
typedef int error1; // 1 is the error value
typedef int errorneg1; // -1 is the error value
typedef int errorn; // 1 or more means error


#ifndef SIGBUS
#define SIGBUS SIGSEGV /* close enough */
#endif

//******************************************************************************
// AUTOLIB
//******************************************************************************

/*
 * Funny-named symbols to tip off 9l to autolink.
 */
#define AUTOLIB(x)	static int __p9l_autolib_ ## x = 1;
#define AUTOFRAMEWORK(x) static int __p9l_autoframework_ ## x = 1;

/*
 * Gcc is too smart for its own good.
 */
#if defined(__GNUC__)
#	undef strcmp	/* causes way too many warnings */
#	if __GNUC__ >= 4 || (__GNUC__==3 && !defined(__APPLE_CC__) && !defined(__MINGW32__))
#		undef AUTOLIB
#		define AUTOLIB(x) int __p9l_autolib_ ## x __attribute__ ((weak));
#		undef AUTOFRAMEWORK
#		define AUTOFRAMEWORK(x) int __p9l_autoframework_ ## x __attribute__ ((weak));
#	else
#		undef AUTOLIB
#		define AUTOLIB(x) static int __p9l_autolib_ ## x __attribute__ ((unused));
#		undef AUTOFRAMEWORK
#		define AUTOFRAMEWORK(x) static int __p9l_autoframework_ ## x __attribute__ ((unused));
#	endif
#endif

//******************************************************************************
// Postlude
//******************************************************************************

#endif
