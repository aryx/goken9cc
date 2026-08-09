
// a bool type! not part of C89; had to wait for C23 to have
// 'bool', 'true', and 'false' be proper language keywords.

// 'bool' is now needed to compile {mk,rc,5c,8c,...} which
// comes from pad's principia which use a few extra C types
// (I like types, and I especially don't like abusing ints for everything)

// Note that using 'u8' for bool and not 'int' has consequences! DO NOT
// transform what you think is a bool like 'int flag;' to 'bool flag;' when
// this variable is actually used in arithmetic context. It led to
// many regressions.
// alt: typedef int bool; // would cause less regressions, but not as clean
typedef u8 bool;
enum _bool {
	false = 0,
	true = 1,
};

//typedef uchar bool_byte;
