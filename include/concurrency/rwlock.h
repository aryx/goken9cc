
struct RWLock {
    Lock    lock;
    int readers;    /* number of readers */
    int writer;     /* number of writers */

    QLp *head;      /* list of waiting processes */
    QLp *tail;
};

extern  void    rlock(RWLock*);
extern  void    runlock(RWLock*);
extern  int     canrlock(RWLock*);
extern  void    wlock(RWLock*);
extern  void    wunlock(RWLock*);
extern  int     canwlock(RWLock*);
