// Buffered input/output library.
#ifndef _BIO_H_
#define _BIO_H_ 1

#ifdef AUTOLIB
AUTOLIB(bio)
#endif

#include <fcntl.h>	/* for O_RDONLY, O_WRONLY */

enum
{
	Bsize		= 8*1024,
	Bungetsize	= 4,		/* space for ungetc */
	Bmagic		= 0x314159,
	Beof		= -1,
	Bbad		= -2,
};

// BioState
enum
{
	Binactive	= 0,		/* states */
	Bractive,
	Bwactive,
	Bracteof,

	Bend
};

typedef	struct	Biobuf	Biobuf;
struct	Biobuf
{
	int	icount;		/* neg num of bytes at eob */
	int	ocount;		/* num of bytes at bob */
	int	rdline;		/* num of bytes after rdline */
	int	runesize;	/* num of bytes of last getrune */
        // enum<BioState>
	int	state;		/* r/w/inactive */
        //TODO: fdt
	int	fid;		/* open file */
	int	flag;		/* magic if malloc'ed */
	vlong	offset;		/* offset of buffer in file */
	int	bsize;		/* size of buffer */
	unsigned char*	bbuf;		/* pointer to beginning of buffer */
	unsigned char*	ebuf;		/* pointer to end of buffer */
	unsigned char*	gbuf;		/* pointer to good data in buf */
	unsigned char	b[Bungetsize+Bsize];
};

#define	BGETC(bp)\
	((bp)->icount?(bp)->bbuf[(bp)->bsize+(bp)->icount++]:Bgetc((bp)))
#define	BPUTC(bp,c)\
	((bp)->ocount?(bp)->bbuf[(bp)->bsize+(bp)->ocount++]=(c),0:Bputc((bp),(c)))
#define	BOFFSET(bp)\
	(((bp)->state==Bractive)?\
		(bp)->offset + (bp)->icount:\
	(((bp)->state==Bwactive)?\
		(bp)->offset + ((bp)->bsize + (bp)->ocount):\
		-1))
#define	BLINELEN(bp)\
	(bp)->rdline
#define	BFILDES(bp)\
	(bp)->fid

Biobuf*	Bopen(char*, int /* open flag (e.g., OWRITE) */);
Biobuf*	Bfdopen(fdt, int); // not in plan9 bio.h

int	Binit(Biobuf*, fdt, int);
int	Binits(Biobuf*, fdt, int, unsigned char*, int);

long	Bread(Biobuf*, void*, long);
long	Bwrite(Biobuf*, void*, long);
vlong	Bseek(Biobuf*, vlong, int);

int	Bbuffered(Biobuf*);
int	Bfildes(Biobuf*);
int	Bflush(Biobuf*);

int	Bgetc(Biobuf*);
int	Bgetd(Biobuf*, double*);
long	Bgetrune(Biobuf*);
int	Bungetc(Biobuf*);
int	Bungetrune(Biobuf*);
int	Bputc(Biobuf*, int);
int	Bputrune(Biobuf*, long);

int	Blinelen(Biobuf*);
vlong	Boffset(Biobuf*);
void*	Brdline(Biobuf*, int);
char*	Brdstr(Biobuf*, int, int);
int	Bterm(Biobuf*);

int	Bprint(Biobuf*, char*, ...);
int	Bvprint(Biobuf*, char*, va_list);

#endif
