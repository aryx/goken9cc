/* Minimal APE (`ANSI/POSIX Environment`) shim: lets UNIX-style C source
 * build against goken's own libc.a, the way Plan 9's real APE
 * (`/sys/include/ape/stdio.h`) sits in front of its native libc.
 *
 * Every benchs/compcert/*.c program wired up so far includes
 * <stdio.h>, so this one file carries everything the others
 * (stddef.h, stdlib.h, string.h) would otherwise need to declare
 * themselves -- those are deliberately empty, not missing content.
 * This is a real assumption, not an oversight: a program that includes
 * only <stdlib.h> (say) without ever including <stdio.h> would get no
 * declarations at all under this scheme. If that ever happens, either
 * give that one header real content, or (cleaner, if it happens more
 * than once) move the shared parts below into libc.h itself so every
 * APE/*.h can pull them in independently again.
 *
 * No include guard: matches u.h/libc.h's own convention of relying on
 * "included exactly once" rather than defending against it. None of
 * the programs built against this so far include <stdio.h> twice.
 */
#include <u.h>
#include <libc.h>

#define printf print

/* stdout: not a real FILE* (this libc has none) -- just a placeholder
 * token so `putc(c, stdout)` parses; putc() below ignores it entirely
 * and always writes to fd 1, which is the only stream any benchmark
 * wired up so far ever writes to anyway.
 */
#define stdout 1

/* putc: libc.h's print("%c", c) writes exactly one raw byte (dofmt.c's
 * __charfmt, not __runefmt/%C's UTF-8 rune encoding), so this is safe
 * for binary output (e.g. mandelbrot.c's PBM byte stream), not just
 * text -- verified by reading __charfmt's own implementation, not
 * assumed from the verb's name.
 */
#define putc(c, stream) print("%c", (c))

/* stddef.h's NULL: u.h already defines `nil` as ((void*)0); NULL is
 * just the standard C name for the same thing. */
#define NULL nil

/* stdlib.h's atoi/atol/malloc/calloc/free: implemented in
 * lib_core/libc/port/{atol,minimal_malloc}.c but, unlike memset/
 * strlen/etc (already declared by libc.h itself, via core/mem.h and
 * base/str.h), never had a libc.h-level prototype of their own.
 * Explicit prototypes matter here more than they might look: an
 * implicit (undeclared) function call defaults to returning `int`,
 * which would truncate malloc()'s real `void*` return value on any
 * 64-bit arch -- exactly the class of bug
 * docs/claude_notes/notes_libc_selfhost.txt already documents finding
 * elsewhere in this project (syscall arguments truncated through a
 * too-narrow type).
 */
extern int atoi(char *s);
extern long atol(char *s);
extern void *malloc(ulong n);
extern void *calloc(ulong n, ulong size);
extern void free(void *p);
