// Copyright 2009 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <pthread.h>
#include "libcgo.h"

static void* threadentry(void*);

char *environ[] = { 0 };
char *__progname;

void
initcgo(void)
{
}

void
libcgo_sys_thread_start(ThreadStart *ts)
{
	pthread_attr_t attr;
	pthread_t p;
	size_t size;

	pthread_attr_init(&attr);
	pthread_attr_getstacksize(&attr, &size);
	ts->g->stackguard = size;
	pthread_create(&p, &attr, threadentry, ts);
}

static void*
threadentry(void *v)
{
	ThreadStart ts;

	ts = *(ThreadStart*)v;
	free(v);

	ts.g->stackbase = (uintptr)&ts;

	/*
	 * Set specific keys.  On FreeBSD/ELF, the thread local storage
	 * is just before %fs:0.  Our dynamic 6.out's reserve 16 bytes
	 * for the two words g and m at %fs:-16 and %fs:-8.
	 */
	asm volatile (
		"movq %0, %%fs:-16\n"	// MOVL g, -16(FS)
		"movq %1, %%fs:-8\n"	// MOVL m, -8(FS)
		:: "r"(ts.g), "r"(ts.m)
	);
	crosscall_amd64(ts.fn);
	return nil;
}
