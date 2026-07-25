
//builtin: sizeof
// signof?
// typeof?

#define nelem(x)    (sizeof(x)/sizeof((x)[0]))

#define offsetof(s, m)  (ulong)(&(((s*)nil)->m))
