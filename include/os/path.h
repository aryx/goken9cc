
// atl: fd2path here, or plan 9 specific

//TODO? handle the / vs \ of unix vs windows here? like in OCaml fpath library?

// modified in place, so type should really be void cleanname(INOUT char*);
extern	char*	cleanname(char*);

// plan9port specific, for "#9/..." and "#d/..." paths
extern	char*	unsharp(char*);
