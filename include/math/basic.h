
extern  int     abs(int);
extern  double  fabs(double);

extern  double  floor(double);
extern  double  ceil(double);
extern  double  fmod(double, double);

//extern  long    labs(long);

extern  double  frexp(double, int*);
extern  double  ldexp(double, int);
extern  double  modf(double, double*);

#define HUGE    3.4028234e38

extern  double  NaN(void);
extern  double  Inf(int);

extern  int     isNaN(double);
extern  int     isInf(double, int);

extern  double  exp(double);
extern  double  log(double);
extern  double  log10(double);
extern  double  pow(double, double);
extern  double  pow10(int);
extern  double  sqrt(double);

extern  double  hypot(double, double);

//----------------------------------------------------------------------------
// Trigonometry
//----------------------------------------------------------------------------

#define PIO2    1.570796326794896619231e0
#define PI  (PIO2+PIO2)

extern  double  sin(double);
extern  double  cos(double);
extern  double  tan(double);

extern  double  asin(double);
extern  double  acos(double);
extern  double  atan(double);
extern  double  atan2(double, double);

extern  double  sinh(double);
extern  double  cosh(double);
extern  double  tanh(double);
