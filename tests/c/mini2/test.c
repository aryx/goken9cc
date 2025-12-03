typedef	signed char            int8;
typedef	unsigned char			byte;

void
printf(int8 *s, ...)
{
	byte *arg;

	arg = (byte*)(&s+1);
	vprintf(s, arg);
}
