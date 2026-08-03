
typedef int fdt; // file descriptor type

// enum Open_flag, open parameter
#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
// advanced stuff (no O_APPEND, O_CREATE, O_NONBLOCK as in Unix though)
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */
//#define	ODIRECT	128	/* or'ed in, direct access */
//#define	ONONBLOCK 256	/* or'ed in, non-blocking call */
#define	OEXCL	0x1000	/* or'ed in, exclusive use (create only) */
//#define	OLOCK	0x2000	/* or'ed in, lock after opening */
//#define	OAPPEND	0x4000	/* or'ed in, append only */

// alt: in dir.h
// enum Access_flag
#define	AEXIST	0	/* accessible: exists */
#define	AEXEC	1	/* execute access */
#define	AWRITE	2	/* write access */
#define	AREAD	4	/* read access */

// Qid as in uniQue id
typedef struct Qid Qid;
struct Qid {
  uvlong	path;
  ulong	vers;
  // bitset<Qidtype>
  uchar	type;
};

/* bits in Qid.type */
#define QTFILE		0x00		/* plain file */
#define QTDIR		0x80		/* type bit for directories */
// advanced stuff
#define QTAPPEND	0x40		/* type bit for append only files */
#define QTEXCL		0x20		/* type bit for exclusive use files */
#define QTMOUNT		0x10		/* type bit for mounted channel */
#define QTAUTH		0x08		/* type bit for authentication file */
#define QTTMP		0x04		/* type bit for not-backed-up file */
//#define QTSYMLINK	0x02		/* type bit for symbolic link */

// pad's stuff (but it is actually also in stdio.h)
enum Seek_cursor {
    SEEK__START = 0,
    SEEK__CUR = 1,
    SEEK__END = 2,
};


extern	fdt	open(char*, int);
extern	int	close(fdt);

// plan9 specific
// alt: move in os/plan9/file.h?
extern	long	pread(fdt, void*, long, vlong);
extern	long	pwrite(fdt, void*, long, vlong);
// called dup2 in unix?
extern	int	dup(fdt, fdt);

// in <unistd.h>
extern	long	read(fdt, void*, long);
extern	long	write(fdt, void*, long);
extern	vlong	seek(fdt, vlong, int);

//less useful
//extern	long	preadv(fdt, IOchunk*, int, vlong);
//extern	long	pwritev(fdt, IOchunk*, int, vlong);
//extern	long	readv(fdt, IOchunk*, int);
//extern	long	writev(fdt, IOchunk*, int);
//extern	long	readn(fdt, void*, long);

// extern	int	fdflush(int);
