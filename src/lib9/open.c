#define _GNU_SOURCE	/* for Linux O_DIRECT */
#include <u.h>
#define NOPLAN9DEFINES
#include <sys/file.h>
#include <libc.h>
#ifndef O_DIRECT
#define O_DIRECT 0
#endif

//TODO: was more complex in plan9port and was also containing the code
// of dirread(), dirput(), dirget() and so on
int
p9open(char *name, int mode)
{
	int rclose;
	int fd, umode, rdwr;

	rdwr = mode&3;
	umode = rdwr;
	rclose = mode&ORCLOSE;
	mode &= ~(3|ORCLOSE);
	if(mode&OTRUNC){
		umode |= O_TRUNC;
		mode ^= OTRUNC;
	}
	if(mode&ODIRECT){
		umode |= O_DIRECT;
		mode ^= ODIRECT;
	}
	if(mode&OAPPEND){
		umode |= O_APPEND;
		mode ^= OAPPEND;
	}
	if(mode){
		werrstr("mode 0x%x not supported", mode);
		return -1;
	}
	umode |= O_BINARY;
	fd = open(name, umode);
	if(fd >= 0){
		if(rclose)
			remove(name);
	}
	return fd;
}
