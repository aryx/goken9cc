# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

include ../../Make.inc
O:=$(HOST_O)

TARG=8l
HOST_CFLAGS+=-I../ld

OFILES=\
	asm.$O\
	data.$O\
	dwarf.$O\
	elf.$O\
	enam.$O\
	go.$O\
	lib.$O\
	list.$O\
	macho.$O\
	obj.$O\
	optab.$O\
	pass.$O\
	pe.$O\
	prof.$O\
	span.$O\
	symtab.$O\


HFILES=\
	l.h\
	../8l/8.out.h\
	../ld/dwarf.h\
	../ld/elf.h\
	../ld/macho.h\
	../ld/pe.h\

include ../../Make.ccmd

enam.c: 8.out.h
	sh mkenam

CLEANFILES+=enam.c


#%.$O: ../ld/%.c
#	$(HOST_CC) $(HOST_CFLAGS) -c -I. ../ld/$*.c
