/*
 * el/main.c -- driver, mirrors the shape of every other arch's main.c
 * (parse args, read the object file(s), emit).
 */
#include "l.h"

char*	outfile;
Biobuf	obuf;

void
diag(char *fmt, ...)
{
    va_list arg;

    fprint(2, "el: ");
    va_start(arg, fmt);
    vfprint(2, fmt, arg);
    va_end(arg);
    fprint(2, "\n");
    nerrors++;
}

void
errorexit(void)
{
    exits("error");
}

/*
 * claude: `-I symbol=module.field[:sig]` -- see l.h's Import comment
 * for why this exists (the wasm counterpart of 6lg's `-I thunk:sym:lib`
 * dynamic-import flag) and for the optional trailing `:sig` (a Text.sig-
 * shaped string, e.g. "WV" for WASI's proc_exit), needed once a second
 * import shows up that isn't fd_write's own "WWWWW" shape.
 */
static void
addimportflag(char *arg)
{
    char *eq, *dot, *colon;
    char *symname, *module, *field, *sig;

    eq = strchr(arg, '=');
    if(eq == nil) {
        fprint(2, "el: -I needs symbol=module.field[:sig]\n");
        errorexit();
    }
    *eq = '\0';
    symname = arg;
    dot = strrchr(eq+1, '.');
    if(dot == nil) {
        fprint(2, "el: -I needs symbol=module.field[:sig]\n");
        errorexit();
    }
    *dot = '\0';
    module = eq+1;
    field = dot+1;
    sig = nil;
    colon = strchr(field, ':');
    if(colon != nil) {
        *colon = '\0';
        sig = colon+1;
    }
    addimport(symname, module, field, sig);
}

void
main(int argc, char *argv[])
{
    fdt of;

    ARGBEGIN {
    case 'o':
        outfile = ARGF();
        break;
    case 'I':
        addimportflag(ARGF());
        break;
    } ARGEND

    if(*argv == '\0') {
        print("usage: el [-o out] [-I symbol=module.field] file.e ...\n");
        errorexit();
    }
    if(outfile == nil)
        outfile = "e.out";

    /*
     * claude: readobj()'s only per-file state is its own local h[NSYM]
     * symidx cache (reset on each call); hash[]/firsttext/datarelocs
     * are file-scope globals that naturally accumulate across calls,
     * so linking several object files together needs nothing more
     * than calling it once per file -- no archive support (-l libs)
     * yet, see l.h.
     */
    USED(argc);
    for(; *argv != nil; argv++)
        readobj(*argv);
    if(nerrors) {
        fprint(2, "el: %d error(s), not writing %s\n", nerrors, outfile);
        errorexit();
    }

    of = create(outfile, OWRITE, 0775);
    if(of < 0) {
        diag("cannot create %s", outfile);
        errorexit();
    }
    Binit(&obuf, of, OWRITE);

    asmb();
    if(nerrors) {
        fprint(2, "el: %d error(s)\n", nerrors);
        errorexit();
    }
    exits(nil);
}
