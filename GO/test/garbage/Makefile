# Copyright 2010 The Go Authors.  All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

include ../../Make.inc

ALL=\
	parser\
	peano\
	tree\

all: $(addsuffix .out, $(ALL))

%.$O: %.go
	$(GC) $*.go

%.out: %.$O
	$(LD) -o $@ $*.$O

%.bench: %.out
	./$*.out

bench: $(addsuffix .bench, $(ALL))

clean:
	rm -f *.[$(OS)] $(addsuffix .out, $(ALL))

