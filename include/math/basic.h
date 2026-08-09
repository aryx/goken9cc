
// in <stdlib.h> in standard C library

extern  int     abs(int);

/*
 * provided by system <math.h>
 */

extern  double  fabs(double);

extern  double  floor(double);
extern  double  ceil(double);
extern  double  fmod(double, double);

//extern  long    labs(long);

#define HUGE    3.4028234e38

extern  double  NaN(void);
extern  double  Inf(int);

extern  double  exp(double);
extern  double  log(double);
extern  double  log10(double);
extern  double  pow(double, double);
extern  double  pow10(int);
extern  double  sqrt(double);

extern  double  hypot(double, double);

extern  double  ldexp(double, int);
extern  double  modf(double, double*);

// plan 9 specific?
extern  int     isNaN(double);
extern  int     isInf(double, int);
extern  double  frexp(double, int*);
