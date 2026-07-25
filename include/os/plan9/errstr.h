
#define	ERRMAX	128	/* max length of error string */

extern	int	errstr(char*, uint);

extern	void	rerrstr(char*, uint);
extern	void	werrstr(char*, ...);

#pragma	varargck	argpos	werrstr	1
