
// a bool type! not part of C89, appeared in C11?

// bool is now needed to compile {mk,rc,5c,8c,...} which
// comes from pad's principia which use a few extra C types
// (I like types, and I especially don't like abusing ints for everything)

// Note that using 'uint8' for bool and not 'int' has consequences! DO NOT
// transform what you think is a bool like 'int flag;' to a bool when
// this variable is actually used in arithmetic context. It led to
// many regressions.
// alt: typedef int bool; // would cause less regressions, but not as clean
typedef u8 bool;
enum _bool {
	false = 0,
	true = 1,
};

//typedef uchar bool_byte;
