
// IMHO clearer than all those strcmp(...) == -1, == 0, etc.
enum _ord {
  EQ = 0,
  INF = -1,
  SUP = 1,
};

typedef int ord;

// for backward compatibility, but we should refactor the code to use EQ/INF/SUP
#define ORD__EQ EQ
#define ORD__SUP SUP
#define ORD__INF INF
