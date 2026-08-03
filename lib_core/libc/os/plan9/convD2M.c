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

/* convD2M/sizeD2M: pack a Dir into the machine-independent stat buffer
 * Plan9's raw FWSTAT syscall expects -- the write-side mirror of
 * convM2D.c. Ported from principia's lib_core/libc/9sys/convD2M.c; see
 * that file's own comment for why this is Plan9-only in this tree.
 */
uint
sizeD2M(Dir *d)
{
	char *sv[4];
	int i, ns;

	sv[0] = d->name;
	sv[1] = d->uid;
	sv[2] = d->gid;
	sv[3] = d->muid;

	ns = 0;
	for (i = 0; i < 4; i++)
		if (sv[i])
			ns += strlen(sv[i]);

	return STATFIXLEN + ns;
}

uint
convD2M(Dir *d, uchar *buf, uint nbuf)
{
	uchar *p, *ebuf;
	char *sv[4];
	int i, ns, nsv[4], ss;

	if (nbuf < BIT16SZ)
		return 0;

	p = buf;
	ebuf = buf + nbuf;

	sv[0] = d->name;
	sv[1] = d->uid;
	sv[2] = d->gid;
	sv[3] = d->muid;

	ns = 0;
	for (i = 0; i < 4; i++) {
		if (sv[i])
			nsv[i] = strlen(sv[i]);
		else
			nsv[i] = 0;
		ns += nsv[i];
	}

	ss = STATFIXLEN + ns;

	/* set the size before erroring, so the caller can learn how much
	 * space is needed -- length excludes the count field itself.
	 */
	PBIT16(p, ss - BIT16SZ);
	p += BIT16SZ;

	if (ss > nbuf)
		return BIT16SZ;

	PBIT16(p, d->type);
	p += BIT16SZ;
	PBIT32(p, d->dev);
	p += BIT32SZ;

	PBIT8(p, d->qid.type);
	p += BIT8SZ;
	PBIT32(p, d->qid.vers);
	p += BIT32SZ;
	PBIT64(p, d->qid.path);
	p += BIT64SZ;

	PBIT32(p, d->mode);
	p += BIT32SZ;
	PBIT32(p, d->atime);
	p += BIT32SZ;
	PBIT32(p, d->mtime);
	p += BIT32SZ;

	PBIT64(p, d->length);
	p += BIT64SZ;

	for (i = 0; i < 4; i++) {
		ns = nsv[i];
		if (p + ns + BIT16SZ > ebuf)
			return 0;
		PBIT16(p, ns);
		p += BIT16SZ;
		if (ns)
			memmove(p, sv[i], ns);
		p += ns;
	}

	if (ss != p - buf)
		return 0;

	return p - buf;
}
