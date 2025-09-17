#TODO: source mkconfig? or merge with mkconfig?

# bin/ for (promoted) mk and rc and ROOT/{amd,arm}64/... for 6a/6c/6l/...
# called during mk test
export PATH=`pwd`/bin:`pwd`/ROOT/amd64/bin:`pwd`/ROOT/arm64/bin:$PATH
# for mk to find rc
export MKSHELL=`pwd`/bin/rc
# for rc to find its init file
# alt: export PLAN9=`pwd` should work too because of get9root.c
export RCMAIN=`pwd`/etc/rcmain.unix
# for yacc to find its template file
export YACCPAR=`pwd`/etc/yaccpar
