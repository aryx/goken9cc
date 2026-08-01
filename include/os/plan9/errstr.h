
/*
 * error string for %r
 * supplied on per os basis, not part of fmt library
 */

#define	ERRMAX	128	/* max length of error string */ // for errstr()

// This function is bidirectional and can be used to both read and set the
// error string depending how it's called.
extern	int	errstr(char*, uint);

// read but does not clear the error
extern	void	rerrstr(char*, uint);
// set the per-process error string.
extern	void	werrstr(char*, ...);

#pragma	varargck	argpos	werrstr	1
