# bin/ for (promoted) mk and rc and ROOT/arch/$objtype/bin for 6a/6c/6l/...
# called during mk test
. `pwd`/mkconfig
export PATH=`pwd`/bin:`pwd`/ROOT/arch/$objtype/bin:$PATH

# for mk to find rc
export MKSHELL=`pwd`/bin/rc
# disable leak detection when built with --asan; many tools have harmless leaks
export ASAN_OPTIONS=detect_leaks=0
# for mk to use multiple processors (macOS: install nproc homebrew package)
export NPROC=`nproc`

## old: not needed anymore thx to get9root.c and the use of #9/etc/...
## alt: can also be overriden with setting GOROOT
# for rc to find its init file
# export RCMAIN=`pwd`/etc/rcmain.unix
# for yacc to find its template file
# export YACCPAR=`pwd`/etc/yaccpar
