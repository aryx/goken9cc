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

/* readn() (include/os/file.h), plain copy of principia's
 * lib_core/libc/port/readn.c. Found blocking utilities/archive/tar/tar.c
 * while self-hosting utilities/ with goken's own compiler+libc instead
 * of the host bootstrap gcc+lib9.
 */

long
readn(fdt f, void *av, long n)
{
	char *a;
	long m, t;

	a = av;
	t = 0;
	while(t < n){
		m = read(f, a+t, n-t);
		if(m <= 0){
			if(t == 0)
				return m;
			break;
		}
		t += m;
	}
	return t;
}
