
#define	STATMAX	65535U	/* max length of machine-independent stat structure */

extern	int	fstat(int, uchar*, int);
extern	int	stat(char*, uchar*, int);
extern	int	fwstat(int, uchar*, int);
extern	int	wstat(char*, uchar*, int);

extern	Dir*	dirfstat(fdt);
extern	Dir*	dirstat(char*);
extern	int	dirfwstat(int, Dir*);
extern	int	dirwstat(char*, Dir*);
