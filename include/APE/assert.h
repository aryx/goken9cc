/* Empty: every benchs/compcert/*.c program wired up so far also
 * includes <stdio.h>, which pulls in libc.h -- and libc.h already
 * declares assert() (include/debugging/debug.h) under its familiar
 * name, so there is nothing assert.h itself needs to add. See
 * stdio.h's own comment.
 */
