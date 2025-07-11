// Format of the header of Plan9 libraries (e.g., libc.a) produced by 'ar'

// AR magic string
#define	ARMAG	"!<arch>\n"
// size AR magic string
#define	SARMAG	8

// AR file separator magic
#define	ARFMAG	"`\n"

// size AR name entry
#define SARNAME	64

struct	ar_hdr
{
	byte	name[SARNAME];
	byte	date[12];
	byte	uid[6];
	byte	gid[6];
	byte	mode[8];
	byte	size[10];
	byte	fmag[2];
};

// total size AR header
#define	SAR_HDR	(SARNAME+44)
