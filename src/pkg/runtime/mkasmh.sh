#!/bin/sh
# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

set -e

cat <<'EOF'
// Assembly constants.
// AUTOMATICALLY GENERATED BY mkasmh.sh DURING BUILD

EOF

case "$GOARCH" in
386)
	# The offsets 0 and 4 are also known to:
	#	../../cmd/8l/pass.c:/D_GS
	#	../../libcgo/linux_386.c:/^threadentry
	#	../../libcgo/darwin_386.c:/^threadentry
	case "$GOOS" in
	windows)
		echo '#define	get_tls(r)	MOVL 0x2c(FS), r'
		echo '#define	g(r)	0(r)'
		echo '#define	m(r)	4(r)'
		;;
	plan9)
		echo '#define	get_tls(r)'
		echo '#define	g(r)	0xdfffefc0'
		echo '#define	m(r)	0xdfffefc4'
		;;
	linux)
		# On Linux systems, what we call 0(GS) and 4(GS) for g and m
		# turn into %gs:-8 and %gs:-4 (using gcc syntax to denote
		# what the machine sees as opposed to 8l input).
		# 8l rewrites 0(GS) and 4(GS) into these.
		#
		# On Linux Xen, it is not allowed to use %gs:-8 and %gs:-4
		# directly.  Instead, we have to store %gs:0 into a temporary
		# register and then use -8(%reg) and -4(%reg).  This kind
		# of addressing is correct even when not running Xen.
		#
		# 8l can rewrite MOVL 0(GS), CX into the appropriate pair
		# of mov instructions, using CX as the intermediate register
		# (safe because CX is about to be written to anyway).
		# But 8l cannot handle other instructions, like storing into 0(GS),
		# which is where these macros come into play.
		# get_tls sets up the temporary and then g and r use it.
		#
		# The final wrinkle is that get_tls needs to read from %gs:0,
		# but in 8l input it's called 8(GS), because 8l is going to
		# subtract 8 from all the offsets, as described above.
		echo '#define	get_tls(r)	MOVL 8(GS), r'
		echo '#define	g(r)	-8(r)'
		echo '#define	m(r)	-4(r)'
		;;
	*)
		echo '#define	get_tls(r)'
		echo '#define	g(r)	0(GS)'
		echo '#define	m(r)	4(GS)'
		;;
	esac
	;;
amd64)
	# The offsets 0 and 8 are known to:
	#	../../cmd/6l/pass.c:/D_GS
	#	../../libcgo/linux_amd64.c:/^threadentry
	#	../../libcgo/darwin_amd64.c:/^threadentry
	#
	echo '#define	get_tls(r)'
	echo '#define	g(r) 0(GS)'
	echo '#define	m(r) 8(GS)'
	;;
arm)
	echo '#define	g	R10'
	echo '#define	m	R9'
	echo '#define	LR	R14'
	;;
*)
	echo 'unknown $GOARCH: '$GOARCH 1>&2
	exit 1
	;;
esac
echo

awk '
/^aggr G$/ { aggr="g" }
/^aggr M$/ { aggr = "m" }
/^aggr Gobuf$/ { aggr = "gobuf" }
/^}/ { aggr = "" }

#	Gobuf 24 sched;
#	'Y' 48 stack0;
#	'Y' 56 entry;
#	'A' G 64 alllink;
aggr != "" && /^	/ {
	name=$NF;
	sub(/;/, "", name);
	offset=$(NF-1);
	printf("#define %s_%s %s\n", aggr, name, offset);
}
' runtime.acid.$GOARCH

