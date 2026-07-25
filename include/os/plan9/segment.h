/* Segattch */
#define	SG_RONLY	0040	/* read only */
#define	SG_CEXEC	0100	/* detach on exec */

// alt: in ipc.h

extern	void*	segattach(int, char*, void*, ulong);
extern	void*	segbrk(void*, void*);
extern	int	segdetach(void*);
extern	int	segflush(void*, ulong);
extern	int	segfree(void*, ulong);
