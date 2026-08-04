#include <u.h>
#include <libc.h>

/* gmtime()/localtime()/asctime()/ctime()/tm2sec() (include/os/time.h).
 *
 * Every check below uses a fixed epoch value, never the wall clock --
 * this is the same "deterministic assertions, never rely on values
 * that vary per-run" rule proc.c's own timestamp checks had to work
 * around (stat.c's own header comment references it too). Expected
 * fields/strings were cross-checked against Python's
 * datetime.utcfromtimestamp() for each epoch below, not hand-derived.
 *
 * localtime() is not separately exercised: with no /env/timezone file
 * present on this GOOS, it degenerates to gmtime() (port/ctime.c's own
 * header comment), so a dedicated check would just be gmtime()'s again.
 */
void
main(void)
{
	Tm *t;
	char *s;

	/* 1: gmtime(0) -- the epoch itself, and the one Plan9's own comment
	 * calls out by name ("1/1/1970 was Thursday").
	 */
	t = gmtime(0);
	if (t->sec != 0 || t->min != 0 || t->hour != 0) {
		print("gmtime(0): wrong time of day\n");
		exit(1);
	}
	if (t->mday != 1 || t->mon != 0 || t->year != 70) {
		print("gmtime(0): wrong date\n");
		exit(1);
	}
	if (t->wday != 4 || t->yday != 0) {
		print("gmtime(0): wrong wday/yday\n");
		exit(1);
	}
	if (strcmp(t->zone, "GMT") != 0) {
		print("gmtime(0): wrong zone\n");
		exit(1);
	}

	/* 2: ctime(0) -- asctime(gmtime(0)) end to end, exact string.
	 * ct_numb() (port/ctime.c) space-pads a single-digit day of month
	 * ("Jan  1", two spaces), the classic ctime(3) format -- not
	 * zero-padded, caught by an early wrong guess of "Jan 01" here that
	 * a real qemu run of this test disproved.
	 */
	s = ctime(0);
	if (strcmp(s, "Thu Jan  1 00:00:00 GMT 1970\n") != 0) {
		print("ctime(0): wrong string: %s\n", s);
		exit(1);
	}

	/* 3: one day later -- crosses into a new wday/mday, still 1970. */
	s = ctime(86400L);
	if (strcmp(s, "Fri Jan  2 00:00:00 GMT 1970\n") != 0) {
		print("ctime(86400): wrong string: %s\n", s);
		exit(1);
	}

	/* Pre-1970 (negative tim) is NOT exercised here -- gmtime()'s
	 * `hms = (ulong)tim % 86400L; day = (ulong)tim / 86400L;` (ported
	 * verbatim from principia's lib_core/libc/9sys/ctime.c) mixes a
	 * ulong-cast dividend with a plain `long` divisor, so the usual
	 * arithmetic conversions make the whole division unsigned -- hms
	 * can never come back negative, so the `if(hms<0){hms+=86400;
	 * day-=1;}` fix-up right after it is dead code, and day comes back
	 * as a huge wrapped-around positive count instead of a small
	 * negative one. gmtime(-86400) was tried directly (a small isolated
	 * repro, not this test) and confirmed to land in the year 2106, not
	 * 1969. This is upstream Plan9 libc's own behavior, faithfully
	 * reproduced, not a bug this port introduced -- Plan9 systems don't
	 * see pre-1970 clock values in practice, so it was presumably never
	 * exercised there either.
	 */

	/* 4: a date with hour/min/sec all nonzero, in the 2000s (exercises
	 * asctime()'s "20YY" branch) and a leap year (2000) crossed by the
	 * running day-of-year loop long before it.
	 */
	s = ctime(1000000000L);
	if (strcmp(s, "Sun Sep  9 01:46:40 GMT 2001\n") != 0) {
		print("ctime(1000000000): wrong string: %s\n", s);
		exit(1);
	}

	/* 5: tm2sec() is gmtime()'s inverse -- round-trip a handful of
	 * epochs through both and check we land back exactly.
	 */
	if (tm2sec(gmtime(0)) != 0) {
		print("tm2sec(gmtime(0)) round-trip failed\n");
		exit(1);
	}
	if (tm2sec(gmtime(86400L)) != 86400L) {
		print("tm2sec(gmtime(86400)) round-trip failed\n");
		exit(1);
	}
	if (tm2sec(gmtime(1700000000L)) != 1700000000L) {
		print("tm2sec(gmtime(1700000000)) round-trip failed\n");
		exit(1);
	}

	print("ctime ok\n");
	exit(0);
}
