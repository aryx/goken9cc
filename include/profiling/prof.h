
enum Profiling {
    Profoff,        /* No profiling */

    Profuser,       /* Measure user time only (default) */
    Profkernel,     /* Measure user + kernel time */
    Proftime,       /* Measure total time */
    Profsample,     /* Use clock interrupt to sample (default when there is no cycle counter) */
}; /* what */

extern  void    prof(void (*fn)(void*), void *arg, int entries, int what);
