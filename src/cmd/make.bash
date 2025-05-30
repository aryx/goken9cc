#!/usr/bin/env bash
# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

set -e

bash clean.bash

eval $(gomake --no-print-directory -f ../Make.inc go-env)
if [ -z "$O" ]; then
	echo 'missing $O - maybe no Make.$GOARCH?' 1>&2
	exit 1
fi

cd ${O}l
bash mkenam
gomake enam.o
cd ..

# Note: commands written in Go are not listed here.
# They are in ../pkg/Makefile so that they can be built
# after the Go libraries on which they depend.
for i in cc ${O}l ${O}a ${O}c gc ${O}g cov godefs gopack gotest nm prof
do
	echo; echo; echo %%%% making $i %%%%; echo
	cd $i
	gomake install
	cd ..
done
