
struct QLock {
    Lock    lock;
    int locked;

    QLp *head;
    QLp     *tail;
};

extern  void    qlock(QLock*);
extern  void    qunlock(QLock*);
extern  int     canqlock(QLock*);

extern  void    _qlockinit(void* (*)(void*, void*));    /* called only by the thread library */
