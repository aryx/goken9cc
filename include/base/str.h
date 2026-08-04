
/*
 * string routines (provided by system <string.h>)
 */

// memxxx equivalent, but with special handling for '\0' (no need pass ulong)

extern  long    strlen(char*);
extern  int     strcmp(char*, char*);
extern  char*   strcpy(char*, char*);
extern  char*   strchr(char*, int);
extern  char*   strdup(char*);
extern  char*   strstr(char*, char*);
extern  char*   strcat(char*, char*);
extern  char*   strrchr(char*, int);
extern  char*   strncpy(char*, char*, long);
extern  int     cistrcmp(char*, char*);

// less useful?
//extern  char*   strecpy(char*, char*, char*);
//extern  char*   strncat(char*, char*, long);
// claude: needed by lib_strings/libstring/s_rdinstack.c and
// utilities/pipe/*.c -- port/strncmp.c, ported from principia's
// lib_core/libc/port/strncmp.c. Found self-hosting utilities/ with
// goken's own compiler+libc instead of the host bootstrap gcc+lib9.
extern  int     strncmp(char*, char*, long);
//
//extern  char*   strpbrk(char*, char*);
//
//extern  long    strspn(char*, char*);
//extern  long    strcspn(char*, char*);
//
//extern  int     cistrncmp(char*, char*, int);
//extern  char*   cistrstr(char*, char*);
//
//extern  char*   strtok(char*, char*);
// claude: needed by os/plan9/wait.c (Tier 4 process control, docs/
// claude_notes/plan_syscalls.txt) to split await()'s "pid utime stime
// rtime msg" text into fields -- port/tokenize.c, ported from
// principia's lib_core/libc/port/tokenize.c. gettokens() (same file,
// same shape minus '\''-quoting) came along with it but has no caller
// in this tree yet.
extern  int     tokenize(char*, char**, int);
extern  int     gettokens(char*, char**, int, char*);

