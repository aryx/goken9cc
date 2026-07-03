// Complement of a sub-word unsigned value: C promotes to int, so
// ~c must operate on 32 bits (kencc emits NOTL). The principia 8c
// emits NOTB + MOVBLZX which computes (uchar)~c zero-extended
// instead: for c==0, ~c & 0xffff is 0xffff in C but 0xff with the
// principia code. Found on the principia corpus (utilities/text/misc/
// tr.c, sort.c, libimg/ico.c, snoopy/icmp.c); the cause was OCOM
// calling arith(n, false) instead of arith(n, true) in cc/com.c.

int
f(unsigned char c)
{
	return ~c & 0xffff;
}
