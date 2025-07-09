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
	char	name[SARNAME];
	char	date[12];
	char	uid[6];
	char	gid[6];
	char	mode[8];
	char	size[10];
	char	fmag[2];
};

// total size AR header
#define	SAR_HDR	(SARNAME+44)
