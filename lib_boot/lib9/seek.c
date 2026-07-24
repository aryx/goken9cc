#include <u.h>
#include <libc.h>

vlong
seek(fdt fd, vlong offset, int whence)
{
	return lseek(fd, offset, whence);
}
