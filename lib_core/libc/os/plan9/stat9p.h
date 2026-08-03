/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* The tiny slice of Plan9's fcall.h this GOOS's convM2D.c/convD2M.c
 * need to (un)pack the wire-format stat buffer FSTAT/FWSTAT actually
 * exchange -- NOT a general fcall.h port (no Twalk/Rread/9P message
 * framing here, nothing else in this tree needs that yet). Only
 * os/plan9/stat.c includes this; see include/os/stat.h's own comment
 * on why this wire format is Plan9-internal rather than a portable API.
 * Bit-getter/putter names and byte layout match principia's
 * lib_core/libc/9sys/{convM2D,convD2M}.c and include/ipc/fcall.h
 * exactly, so a future full fcall.h port can drop this file outright.
 */

#define BIT8SZ		1
#define BIT16SZ		2
#define BIT32SZ		4
#define BIT64SZ		8
#define QIDSZ		(BIT8SZ+BIT32SZ+BIT64SZ)
/* STATFIXLEN: the fixed-length part of a packed stat buffer -- leading
 * 16-bit count, a Qid, mode/atime/mtime (32 bit each), length (64 bit),
 * and four 16-bit string-length prefixes (name/uid/gid/muid) whose
 * actual string bytes follow and are NOT included here.
 */
#define STATFIXLEN	(BIT16SZ+QIDSZ+5*BIT16SZ+4*BIT32SZ+1*BIT64SZ)

#define GBIT8(p)	((p)[0])
#define GBIT16(p)	((p)[0]|((p)[1]<<8))
#define GBIT32(p)	((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))
#define GBIT64(p)	((uint)((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24)) |\
			 ((uvlong)((p)[4]|((p)[5]<<8)|((p)[6]<<16)|((p)[7]<<24)) << 32))

#define PBIT8(p,v)	(p)[0]=(v)
#define PBIT16(p,v)	do{(p)[0]=(v);(p)[1]=(v)>>8;}while(0)
#define PBIT32(p,v)	do{(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24;}while(0)
#define PBIT64(p,v)	do{(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24;\
			 (p)[4]=(v)>>32;(p)[5]=(v)>>40;(p)[6]=(v)>>48;(p)[7]=(v)>>56;}while(0)

extern uint	convM2D(uchar*, uint, Dir*, char*);
extern uint	convD2M(Dir*, uchar*, uint);
extern uint	sizeD2M(Dir*);
extern int	statcheck(uchar*, uint);
