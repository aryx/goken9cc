
// a bool type! not part of C89, appeared in C11?
// Using int8 for a bool would be more compact but has consequences
// when using it in arithmetic context.
// Safer to use a full int for now.

typedef int bool;
enum _bool {
  false = 0,
  true = 1
};


typedef uchar bool_byte;
