
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
#define	OEXCL	0x1000	/* or'ed in, exclusive use (create only) */

// enum Access_flag
#define	AEXIST	0	/* accessible: exists */
#define	AEXEC	1	/* execute access */
#define	AWRITE	2	/* write access */
#define	AREAD	4	/* read access */

// Qid as in uniQue id
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

// pad's stuff (but it is actually also in stdio.h)
enum Seek_cursor {
    SEEK__START = 0,
    SEEK__CUR = 1,
    SEEK__END = 2,
};


extern	fdt	open(char*, int);
extern	int	close(fdt);
extern	long	pread(fdt, void*, long, vlong);
extern	long	pwrite(fdt, void*, long, vlong);
extern	int	dup(int, int);

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
