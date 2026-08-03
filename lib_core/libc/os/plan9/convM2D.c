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
#include "stat9p.h"

/* convM2D: unpack the machine-independent stat buffer Plan9's raw
 * FSTAT/FWSTAT syscalls exchange into a Dir. Ported from principia's
 * lib_core/libc/9sys/convM2D.c -- see include/os/stat.h's own comment
 * for why this wire-format code is Plan9-only in this tree (every
 * other GOOS builds a Dir straight from its native stat struct, no
 * marshaling at all). Only os/plan9/stat.c calls this.
 */
static char nullstring[] = "";

uint
convM2D(uchar *buf, uint nbuf, Dir *d, char *strs)
{
	uchar *p, *ebuf;
	char *sv[4];
	int i, ns;

	if (nbuf < STATFIXLEN)
		return 0;

	p = buf;
	ebuf = buf + nbuf;

	p += BIT16SZ;	/* ignore leading size */
	d->type = GBIT16(p);
	p += BIT16SZ;
	d->dev = GBIT32(p);
	p += BIT32SZ;
	d->qid.type = GBIT8(p);
	p += BIT8SZ;
	d->qid.vers = GBIT32(p);
	p += BIT32SZ;
	d->qid.path = GBIT64(p);
	p += BIT64SZ;
	d->mode = GBIT32(p);
	p += BIT32SZ;
	d->atime = GBIT32(p);
	p += BIT32SZ;
	d->mtime = GBIT32(p);
	p += BIT32SZ;
	d->length = GBIT64(p);
	p += BIT64SZ;

	for (i = 0; i < 4; i++) {
		if (p + BIT16SZ > ebuf)
			return 0;
		ns = GBIT16(p);
		p += BIT16SZ;
		if (p + ns > ebuf)
			return 0;
		if (strs) {
			sv[i] = strs;
			memmove(strs, p, ns);
			strs += ns;
			*strs++ = '\0';
		}
		p += ns;
	}

	if (strs) {
		d->name = sv[0];
		d->uid = sv[1];
		d->gid = sv[2];
		d->muid = sv[3];
	} else {
		d->name = nullstring;
		d->uid = nullstring;
		d->gid = nullstring;
		d->muid = nullstring;
	}

	return p - buf;
}
