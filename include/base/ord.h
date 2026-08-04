
enum _ord {
  EQ = 0,
  INF = -1,
  SUP = 1,
};

typedef int ord;

// claude: ORD__EQ/ORD__SUP/ORD__INF -- the older, longer names
// BOOT/include/u.h's own copy of this enum still uses (and that
// header can't safely gain the short EQ/INF/SUP names instead:
// utilities/calc/hoc/hoc.y declares its own yacc token named EQ,
// #include's <libc.h> in the same translation unit, and a global
// `EQ` from u.h collided with it -- confirmed by trying exactly that
// and getting "expected identifier before numeric constant" out of
// gcc). Adding the long names here instead, matching the direction
// tar.c/gzip.c/gunzip.c (untouched, still use ORD__EQ) already need
// working under BOTH the BOOT/gcc bootstrap build and goken's own
// self-hosted compilers -- verified no existing goken-native source
// (compilers/, linkers/, lib_core/, lib_toolchain/, lib_strings/)
// already uses this longer name for anything else.
#define ORD__EQ EQ
#define ORD__SUP SUP
#define ORD__INF INF
