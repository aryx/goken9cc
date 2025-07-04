# bin/ for (promoted) mk and rc and ROOT/amd64/... for 6a/6c/6l/...
# called during mk test
export PATH=`pwd`/bin:`pwd`/ROOT/amd64/bin:$PATH
# for mk to find rc
export MKSHELL=`pwd`/bin/rc
# for rc to find its init file
export RCMAIN=`pwd`/etc/rcmain.unix
