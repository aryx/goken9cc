
// memxxx equivalent, but with special handling for '\0' (no need pass ulong)

extern  long    strlen(char*);
extern  int     strcmp(char*, char*);
extern  char*   strcpy(char*, char*);
extern  char*   strchr(char*, int);
extern  char*   strdup(char*);
extern  char*   strstr(char*, char*);
extern  char*   strcat(char*, char*);

// less useful?
//extern  char*   strecpy(char*, char*, char*);
//extern  char*   strncat(char*, char*, long);
//extern  char*   strncpy(char*, char*, long);
//extern  int     strncmp(char*, char*, long);
//extern  char*   strrchr(char*, int);
//
//extern  char*   strpbrk(char*, char*);
//
//extern  long    strspn(char*, char*);
//extern  long    strcspn(char*, char*);
//
//extern  int     cistrncmp(char*, char*, int);
//extern  int     cistrcmp(char*, char*);
//extern  char*   cistrstr(char*, char*);
//
//extern  char*   strtok(char*, char*);
//extern  int     tokenize(char*, char**, int);

