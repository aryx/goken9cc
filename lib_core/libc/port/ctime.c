/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#include <u.h>
#include <libc.h>

/* gmtime()/localtime()/asctime()/ctime() (include/os/time.h), ported
 * near-verbatim from principia's lib_core/libc/9sys/ctime.c -- pure
 * calendar arithmetic plus one optional read() of a Plan9-shaped
 * "/env/timezone" config file, no other GOOS-specific dependency, so
 * this lives in port/ rather than os/$GOOS/ like dirfstat() had to.
 *
 * On every GOOS other than plan9, /env/timezone simply doesn't exist:
 * open() fails, readtimezone() falls through to its error label, and
 * localtime() becomes a GMT-only alias for gmtime() -- the same
 * graceful-degradation this tree already relies on elsewhere (dirread()
 * skipping "." / ".." rather than assuming a specific getdents shape).
 * Not a "too Plan9-specific" API-shape decision like dirfstat()'s
 * design (see docs/claude_notes/plan_syscalls.txt) -- just an optional
 * config file callers on POSIX/Windows never need to provide.
 */

static char dmsize[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int dysize(int);
static void ct_numb(char*, int);

#define TZSIZE 150
static void readtimezone(void);
static int rd_name(char**, char*);
static int rd_long(char**, long*);

static
struct
{
	char stname[4];
	char dlname[4];
	long stdiff;
	long dldiff;
	long dlpairs[TZSIZE];
} timezone;

char*
ctime(long t)
{
	return asctime(localtime(t));
}

Tm*
localtime(long tim)
{
	Tm *ct;
	long t, *p;
	int dlflag;

	if (timezone.stname[0] == 0)
		readtimezone();
	t = tim + timezone.stdiff;
	dlflag = 0;
	for (p = timezone.dlpairs; *p; p += 2)
		if (t >= p[0])
		if (t < p[1]) {
			t = tim + timezone.dldiff;
			dlflag++;
			break;
		}
	ct = gmtime(t);
	if (dlflag) {
		strcpy(ct->zone, timezone.dlname);
		ct->tzoff = timezone.dldiff;
	} else {
		strcpy(ct->zone, timezone.stname);
		ct->tzoff = timezone.stdiff;
	}
	return ct;
}

Tm*
gmtime(long tim)
{
	int d0, d1;
	long hms, day;
	static Tm xtime;

	/* break initial number into days */
	hms = (ulong)tim % 86400L;
	day = (ulong)tim / 86400L;
	if (hms < 0) {
		hms += 86400L;
		day -= 1;
	}

	/* generate hours:minutes:seconds */
	xtime.sec = hms % 60;
	d1 = hms / 60;
	xtime.min = d1 % 60;
	d1 /= 60;
	xtime.hour = d1;

	/* day is the day number. generate day of the week.
	 * the addend is 4 mod 7 (1/1/1970 was Thursday)
	 */
	xtime.wday = (day + 7340036L) % 7;

	/* year number */
	if (day >= 0)
		for (d1 = 1970; day >= dysize(d1); d1++)
			day -= dysize(d1);
	else
		for (d1 = 1970; day < 0; d1--)
			day += dysize(d1-1);
	xtime.year = d1-1900;
	xtime.yday = d0 = day;

	/* generate month */
	if (dysize(d1) == 366)
		dmsize[1] = 29;
	for (d1 = 0; d0 >= dmsize[d1]; d1++)
		d0 -= dmsize[d1];
	dmsize[1] = 28;
	xtime.mday = d0 + 1;
	xtime.mon = d1;
	strcpy(xtime.zone, "GMT");
	return &xtime;
}

char*
asctime(Tm *t)
{
	char *ncp;
	static char cbuf[30];

	strcpy(cbuf, "Thu Jan 01 00:00:00 GMT 1970\n");
	ncp = &"SunMonTueWedThuFriSat"[t->wday*3];
	cbuf[0] = *ncp++;
	cbuf[1] = *ncp++;
	cbuf[2] = *ncp;
	ncp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[t->mon*3];
	cbuf[4] = *ncp++;
	cbuf[5] = *ncp++;
	cbuf[6] = *ncp;
	ct_numb(cbuf+8, t->mday);
	ct_numb(cbuf+11, t->hour+100);
	ct_numb(cbuf+14, t->min+100);
	ct_numb(cbuf+17, t->sec+100);
	ncp = t->zone;
	cbuf[20] = *ncp++;
	cbuf[21] = *ncp++;
	cbuf[22] = *ncp;
	if (t->year >= 100) {
		cbuf[24] = '2';
		cbuf[25] = '0';
	}
	ct_numb(cbuf+26, t->year+100);
	return cbuf;
}

static int
dysize(int y)
{
	if (y%4 == 0 && (y%100 != 0 || y%400 == 0))
		return 366;
	return 365;
}

static void
ct_numb(char *cp, int n)
{
	cp[0] = ' ';
	if (n >= 10)
		cp[0] = (n/10)%10 + '0';
	cp[1] = n%10 + '0';
}

static void
readtimezone(void)
{
	char buf[TZSIZE*11+30], *p;
	int i;

	memset(buf, 0, sizeof(buf));
	i = open("/env/timezone", 0);
	if (i < 0)
		goto error;
	if (read(i, buf, sizeof(buf)) >= sizeof(buf)) {
		close(i);
		goto error;
	}
	close(i);
	p = buf;
	if (rd_name(&p, timezone.stname))
		goto error;
	if (rd_long(&p, &timezone.stdiff))
		goto error;
	if (rd_name(&p, timezone.dlname))
		goto error;
	if (rd_long(&p, &timezone.dldiff))
		goto error;
	for (i = 0; i < TZSIZE; i++) {
		if (rd_long(&p, &timezone.dlpairs[i]))
			goto error;
		if (timezone.dlpairs[i] == 0)
			return;
	}

error:
	timezone.stdiff = 0;
	strcpy(timezone.stname, "GMT");
	timezone.dlpairs[0] = 0;
}

static int
rd_name(char **f, char *p)
{
	int c, i;

	for (;;) {
		c = *(*f)++;
		if (c != ' ' && c != '\n')
			break;
	}
	for (i = 0; i < 3; i++) {
		if (c == ' ' || c == '\n')
			return 1;
		*p++ = c;
		c = *(*f)++;
	}
	if (c != ' ' && c != '\n')
		return 1;
	*p = 0;
	return 0;
}

static int
rd_long(char **f, long *p)
{
	int c, s;
	long l;

	s = 0;
	for (;;) {
		c = *(*f)++;
		if (c == '-') {
			s++;
			continue;
		}
		if (c != ' ' && c != '\n')
			break;
	}
	if (c == 0) {
		*p = 0;
		return 0;
	}
	l = 0;
	for (;;) {
		if (c == ' ' || c == '\n')
			break;
		if (c < '0' || c > '9')
			return 1;
		l = l*10 + c-'0';
		c = *(*f)++;
	}
	if (s)
		l = -l;
	*p = l;
	return 0;
}
