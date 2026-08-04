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

/* mallocz() (include/core/malloc.h) -- malloc() with an optional
 * zero-fill, on top of this tree's own port/minimal_malloc.c (not a
 * principia port: no reference file there is named mallocz.c, it's
 * folded into a bigger allocator there this tree doesn't share).
 * Found blocking lib_strings/libstring/s_alloc.c, which utilities/
 * files/du.c needs (-lstring) for its Biobuf-backed line reading.
 */
void*
mallocz(ulong size, bool clear)
{
	void *p;

	p = malloc(size);
	if (p != nil && clear)
		memset(p, 0, size);
	return p;
}
