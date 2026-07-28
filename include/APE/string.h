/* Empty: every benchs/compcert/*.c program wired up so far also
 * includes <stdio.h>, which pulls in libc.h -- and libc.h already
 * declares memset/memmove/memcmp/memccpy (core/mem.h) and
 * strlen/strcmp/strcpy/strchr (base/str.h) under their familiar
 * names, so there is nothing string.h itself needs to add. See
 * stdio.h's own comment.
 */
