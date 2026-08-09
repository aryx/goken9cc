

// IMHO clearer interface when using those error types than abusing int
// later: unify all of that to be more consistent!
typedef int error0; // 0 is the error value, so 1 is usually OK value
typedef int error1; // 1 is the error value, so 0 is usually OK value
typedef int errorneg1; // -1 is the error value
typedef int errorn; // 1 or more means error

// the macros to use with the types above
// (use OK_1 for error0, use OK_0 for error1)
#define OK_0 0
#define OK_1 1

#define ERROR_0 0
#define ERROR_1 1
#define ERROR_NEG1 (-1)
