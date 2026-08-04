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

/* pipe() (include/os/ipc.h) -- CreatePipe (winio_amd64.s's own
 * _wincreatepipe), a real Win32 primitive with no design compromise
 * needed, unlike exec.c's fork()/wait() story. The one wrinkle is the
 * same one every other os/windows/*.c file already lives with (see
 * open.c's own header comment): fdt is `int` but a real HANDLE is an
 * 8-byte pointer, so the two handles CreatePipe hands back are
 * truncated into fd[0]/fd[1] the same way open()'s own HANDLE result
 * already is. rd/wr are real 8-byte locals (not int) so
 * _wincreatepipe has honest 8-byte storage to write the untruncated
 * HANDLEs into before this file does the truncation itself, rather
 * than truncating one level down inside the raw stub.
 */

extern int _wincreatepipe(void *rd, void *wr);

int
pipe(int *fd)
{
	vlong rd, wr;

	if(!_wincreatepipe(&rd, &wr))
		return -1;
	fd[0] = (int)rd;
	fd[1] = (int)wr;
	return 0;
}
