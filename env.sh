# bin/ for (promoted) mk and rc and ROOT/{amd,arm}64/... for 6a/6c/6l/...
# called during mk test
export PATH=`pwd`/bin:`pwd`/ROOT/amd64/bin:`pwd`/ROOT/arm64/bin:$PATH

# for mk to find rc
export MKSHELL=`pwd`/bin/rc
# for mk to use multiple processors
export NPROC=`nproc`

# old: not needed anymore thx to get9root.c and the use of #9/etc/...
# alt: can also be overriden with setting GOROOT
# for rc to find its init file
# export RCMAIN=`pwd`/etc/rcmain.unix
# for yacc to find its template file
# export YACCPAR=`pwd`/etc/yaccpar
