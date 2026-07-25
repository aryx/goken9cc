
#define STDIN 0
#define STDOUT 1
#define STDERR 2

struct IOchunk {
 void	*addr;
 ulong	len;
};

#define IOUNIT	32768U	/* default buffer size for 9p io */
