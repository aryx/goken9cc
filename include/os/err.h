
// print human-readable error message for UNIX global errno (errno is an int)
extern  void    perror(char*);

// will internally call exits(). TODO? declare hook _sysfatal() here?
extern  void    sysfatal(char*, ...);

#pragma varargck    argpos  sysfatal    1
