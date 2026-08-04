
extern  char*   getenv(char*);
extern  int     putenv(char*, char*);

/* claude: the whole environment as a nil-terminated array of
 * "NAME=value" strings -- exposed so callers that need to enumerate
 * every variable (e.g. a self-hosted mk's readenv()/exportenv(),
 * currently done via dirread() on Plan9's own /env, not portable to
 * any other GOOS) have a real portable primitive instead. Was already
 * computed internally by port/getenv.c's own getenv(); this just makes
 * that walk callable directly. Returns a pointer INTO live state, not
 * a copy -- do not free it, and treat it as invalidated by the next
 * putenv() call (same "no aliasing guarantees across a mutation"
 * contract environ(3) has on a real POSIX system).
 */
extern  char**  environ(void);	/* nil-terminated array of "NAME=value" */
