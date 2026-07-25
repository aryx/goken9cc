
// a similar structure is defined in the kernel!
struct Dir {
 /* system-modified data */
 ushort	type;	/* server type */
 uint	dev;	/* server subtype */

 /* file data */
 Qid	qid;	/* unique id from server */

 ulong	mode;	/* permissions */
 ulong	atime;	/* last read time */
 ulong	mtime;	/* last write time */
 vlong	length;	/* file length */
 char	*name;	/* last element of path */

 char	*uid;	/* owner name */
 char	*gid;	/* group name */
 char	*muid;	/* last modifier name */
};

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMREAD		0x4		/* mode bit for read permission */
#define DMWRITE		0x2		/* mode bit for write permission */
#define DMEXEC		0x1		/* mode bit for execute permission */
// advanced stuff
#define DMAPPEND	0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMAUTH		0x08000000	/* mode bit for authentication file */
#define DMTMP		0x04000000	/* mode bit for non-backed-up files */

#define	DIRMAX	(sizeof(Dir)+STATMAX)	/* max length of Dir structure */

extern	int	create(char*, int, ulong);
extern	int	remove(char*);
extern	int	chdir(char*);
extern	int	fd2path(fdt, char*, int);

extern	long	dirread(int, Dir**);
extern	void	nulldir(Dir*);
extern	long	dirreadall(int, Dir**);

extern  char*   getwd(char*, int);
extern	int	access(char*, int); // ???
extern	bool	fileexists(char*); // new: used to be in linkers/

