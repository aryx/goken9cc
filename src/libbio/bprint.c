#include	<u.h>
#include	<libc.h>
#include	<bio.h>

int
Bprint(Biobuf *bp, char *fmt, ...)
{
	int n;
	va_list arg;

	va_start(arg, fmt);
	n = Bvprint(bp, fmt, arg);
	va_end(arg);
	return n;
}

static int
bflush(Fmt *f)
{
	Biobuf *bp;
	
	if(f->stop == nil)
		return 0;

	bp = f->farg;
	bp->ocount = (char*)f->to - (char*)f->stop;
	if(Bflush(bp) < 0) {
		f->stop = nil;
		f->to = nil;
		return 0;
	}
	f->to = (char*)f->stop + bp->ocount;
	
	return 1;
}

int
Bvprint(Biobuf *bp, char *fmt, va_list arg)
{
	int n;
	Fmt f;
	
	memset(&f, 0, sizeof f);
	fmtlocaleinit(&f, nil, nil, nil);
	f.stop = bp->ebuf;
	f.to = (char*)f.stop + bp->ocount;
	f.flush = bflush;
	f.farg = bp;

	n = fmtvprint(&f, fmt, arg);

	if(f.stop != nil)
		bp->ocount = (char*)f.to - (char*)f.stop;

	return n;
}
