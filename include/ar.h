// Format of the header of Plan9 libraries (e.g., libc.a) produced by 'ar'

// ar magic string
#define	ARMAG	"!<arch>\n"
// size ar magic string
#define	SARMAG	8

// ar file separator magic
#define	ARFMAG	"`\n"

// size ar name entry
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

// total size ar header
#define	SAR_HDR	(SARNAME+44)
