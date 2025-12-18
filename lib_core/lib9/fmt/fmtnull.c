#include <u.h>
#include <libc.h>
#include "fmtdef.h"

/*
 * Absorb output without using resources.
 */
static Rune nullbuf[32];

static int
__fmtnullflush(Fmt *f)
{
	f->to = nullbuf;
	f->nfmt = 0;
	return 0;
}

int
fmtnullinit(Fmt *f)
{
	memset(f, 0, sizeof *f);
	f->runes = 1;
	f->start = nullbuf;
	f->to = nullbuf;
	f->stop = nullbuf+nelem(nullbuf);
	f->flush = __fmtnullflush;
	fmtlocaleinit(f, nil, nil, nil);
	return 0;
}

