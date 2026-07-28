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

/* putchar: same as putc(c, stdout), just without the (unused) stream
 * argument. */
#define putchar(c) print("%c", (c))

/* stddef.h's NULL: u.h already defines `nil` as ((void*)0); NULL is
 * just the standard C name for the same thing. */
#define NULL nil

/* stddef.h's size_t: `ulong`, matching malloc/calloc/free's own
 * parameter types (include/core/mem.h) -- an existing, already-decided
 * project convention (predates this file), not this shim's to
 * override. Yes, that caps a single object's declared size at 4GB in
 * principle (`long` is only 4 bytes on this compiler even on 64-bit
 * arches -- see docs/claude_notes/notes_libc_selfhost.txt's vlong
 * writeup), but nothing built against this libc needs more than a few
 * MB, and picking a wider size_t here would just mean every call site
 * silently truncates back down to ulong at the actual malloc() call
 * anyway -- inconsistent, not safer.
 */
typedef ulong size_t;

/* stdlib.h's atoi/atol: implemented in lib_core/libc/port/atol.c but,
 * unlike malloc/free (include/core/mem.h) or memset/strlen/etc
 * (include/core/mem.h, include/base/str.h), never had a libc.h-level
 * prototype of their own. Worth having explicitly: an implicit
 * (undeclared) function call defaults to returning `int`, which is
 * technically already atoi's/atol's own width for atoi, but not
 * atol's (`long` return silently truncated to `int` without a real
 * declaration) -- see docs/claude_notes/notes_libc_selfhost.txt for
 * the syscall-layer version of this same class of bug.
 */
extern int atoi(char *s);
extern long atol(char *s);
