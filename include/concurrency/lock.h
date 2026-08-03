
typedef struct Lock Lock;
struct Lock {
    long    key;
    long    sem;
};

extern  void    lock(Lock*);
extern  void    unlock(Lock*);
extern  int     canlock(Lock*);
