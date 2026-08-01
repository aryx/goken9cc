
// in <stdlib.h>

//DEPRECATED: time-of-check vs time-of-use (TOCTOU) race condition
// use mkstemp() below instead
extern  char*   mktemp(char*);

// The modern safe one; creates the tmp file atomically with the check
// for existence. 's' stands for secure. Note that it modifies in place
// the template parameter, so you can recover the actual tmp filename
// from it.
//extern fdt mkstemp(char*);

// ??
//extern	int	opentemp(char*);
