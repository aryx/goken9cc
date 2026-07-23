/*
 * ec/list.c -- debug print-format verb installers, and (via #define
 * EXTERN) the actual storage for gc.h's EXTERN-declared globals, the
 * same trick every other arch's list.c uses.
 *
 * claude: minimal on purpose -- nothing in ec's own code currently
 * triggers these (no -g/-G debug-dump call sites like ic's gins()/
 * gopcode() have), so they exist only so the shared driver's
 * `#pragma varargck` declarations and any future debug output have
 * something real to call, not full disassembly listings.
 */
#define EXTERN
#include "gc.h"

void
listinit(void)
{
	fmtinstall('A', Aconv);
	fmtinstall('P', Pconv);
	fmtinstall('D', Dconv);
	fmtinstall('N', Nconv);
	fmtinstall('S', Sconv);
	fmtinstall('B', Bconv);
}

int
Aconv(Fmt *fp)
{
	int a;

	a = va_arg(fp->args, int);
	if(a < 0 || a >= ALAST)
		return fmtprint(fp, "A??%d", a);
	return fmtprint(fp, "A%d", a);
}

int
Dconv(Fmt *fp)
{
	Adr *a;

	a = va_arg(fp->args, Adr*);
	return fmtprint(fp, "D(type=%d,name=%d,off=%ld)", a->type, a->name, a->offset);
}

int
Nconv(Fmt *fp)
{
	return Dconv(fp);
}

int
Pconv(Fmt *fp)
{
	Prog *p;

	p = va_arg(fp->args, Prog*);
	return fmtprint(fp, "(%A %N %N)", p->as, &p->from, &p->to);
}

int
Sconv(Fmt *fp)
{
	char *s;

	s = va_arg(fp->args, char*);
	return fmtprint(fp, "%s", s);
}

int
Bconv(Fmt *fp)
{
	USED(fp);
	return fmtprint(fp, "");
}
