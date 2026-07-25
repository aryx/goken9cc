
struct Rendez {
    QLock   *l;

    QLp *head;
    QLp *tail;
};

extern  void    rsleep(Rendez*);    /* unlocks r->l, sleeps, locks r->l again */
extern  int     rwakeup(Rendez*);

extern  int     rwakeupall(Rendez*);



extern	void*	rendezvous(void*, void*);
