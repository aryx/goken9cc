
enum
{
    PNPROC      = 1,
    PNGROUP     = 2,
};

#define	NCONT	0	/* continue after note */
#define	NDFLT	1	/* terminate after note */
#define	NSAVE	2	/* clear note but hold state */
#define	NRSTR	3	/* restore saved state */

extern	int	noted(int);
extern	int	notify(void(*)(void*, char*));

extern  int     postnote(int, int, char *);
extern  int     atnotify(int(*)(void*, char*), int);
