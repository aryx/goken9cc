
#define	STATMAX	65535U	/* max length of machine-independent stat structure */

/* claude: dirfstat()/dirfwstat() are the real, per-OS implementation --
 * every actual caller in this tree (linkers/ar/ar.c, debuggers/acid/)
 * only ever calls the Dir*-returning forms, never a raw byte-buffer
 * stat/fstat. So unlike Plan9 itself, this tree does NOT expose
 * stat(char*,uchar*,int)/fstat(int,uchar*,int)/wstat/fwstat as a
 * portable API: that machine-independent wire-buffer shape is only
 * meaningful on Plan9, where the kernel's own FSTAT/FWSTAT syscalls
 * genuinely speak it (see os/plan9/stat.c, the one file that still
 * needs convM2D/convD2M to unpack/pack that format). On linux/darwin/
 * windows, os/$GOOS/stat.c builds a Dir directly from that OS's native
 * stat data -- no intermediate marshal step, so no convM2D/convD2M
 * needed there at all.
 *
 * dirstat()/dirwstat() (by path) are portable on top of dirfstat()/
 * dirfwstat() (by fd) -- see port/dirstat.c and port/dirwstat.c -- so
 * every GOOS only has to implement the fd-based pair.
 */
extern	Dir*	dirfstat(fdt);
extern	Dir*	dirstat(char*);
extern	int	dirfwstat(int, Dir*);
extern	int	dirwstat(char*, Dir*);
