#!/usr/bin/env bash
# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

eval $(gomake --no-print-directory -f ../Make.inc go-env)

OUT="Make.deps"
TMP="Make.deps.tmp"

if [ -f $OUT ] && ! [ -w $OUT ]; then
	echo "$0: $OUT is read-only; aborting." 1>&2
	exit 1
fi

# Get list of directories from Makefile
dirs=$(sed '1,/^DIRS=/d; /^$/,$d; s/\\//g' Makefile)
dirpat=$(echo $dirs C | sed 's/ /|/g; s/.*/^(&)$/')

for dir in $dirs; do (
	cd $dir || exit 1

	sources=$(sed -n 's/^[ 	]*\([^ 	]*\.go\)[ 	]*\\*[ 	]*$/\1/p' Makefile)
	sources=$(echo $sources | sed 's/\$(GOOS)/'$GOOS'/g')
	sources=$(echo $sources | sed 's/\$(GOARCH)/'$GOARCH'/g')
	# /dev/null here means we get an empty dependency list if $sources is empty
	# instead of listing every file in the directory.
	sources=$(ls $sources /dev/null 2> /dev/null)  # remove .s, .c, etc.

	deps=$(
		sed -n '/^import.*"/p; /^import[ \t]*(/,/^)/p' $sources /dev/null |
		cut -d '"' -f2 |
		grep -E "$dirpat" |
		grep -v "^$dir\$" |
		sed 's/$/.install/' |
		sed 's;^C\.install;../cmd/cgo.install;' |
		sort -u
	)

	echo $dir.install: $deps
) done > $TMP

mv $TMP $OUT
