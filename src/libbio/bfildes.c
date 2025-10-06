#include	<u.h>
#include	<libc.h>
#include	<bio.h>

int
Bfildes(Biobuf *bp)
{

	return bp->fid;
}
