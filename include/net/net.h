
#define NETPATHLEN 40

extern  int     dial(char*, char*, char*, int*);
extern  int     accept(int, char*);
extern  int     announce(char*, char*);
extern  int     listen(char*, char*);

extern  void    setnetmtpt(char*, int, char*);
extern  int     hangup(int);
extern  char*   netmkaddr(char*, char*, char*);
extern  int     reject(int, char*, char*);

struct NetConnInfo {
    char    *dir;       /* connection directory */
    char    *root;      /* network root */
    char    *spec;      /* binding spec */
    char    *lsys;      /* local system */
    char    *lserv;     /* local service */
    char    *rsys;      /* remote system */
    char    *rserv;     /* remote service */
    char    *laddr;     /* local address */
    char    *raddr;     /* remote address */
};

extern  NetConnInfo*    getnetconninfo(char*, int);
extern  void            freenetconninfo(NetConnInfo*);
