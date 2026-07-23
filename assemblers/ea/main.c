/*s: ea/main.c */
#include "a.h"

int		assemble(char*);
void	cclean(void);

/*
 * claude: modeled on assemblers/5a/main.c; the only wasm-specific
 * bits are thechar/thestring and cclean()'s outcode() call (ea's
 * outcode has no arm-style cond parameter -- see a.h).
 */
void
main(int argc, char *argv[])
{
    errorn err;
    char *p;
    int nout, nproc, status;
    int i, c;

    thechar = 'e';
    thestring = "wasm";

    cinit();
    include[ninclude++] = ".";
    memset(debug, false, sizeof(debug));

    ARGBEGIN {
    case 'o':
        outfile = ARGF();
        break;
    case 'I':
        p = ARGF();
        setinclude(p);
        break;
    case 'D':
        p = ARGF();
        if(p)
            Dlist[nDlist++] = p;
        break;
    default:
        c = ARGC();
        if(c >= 0 || c < sizeof(debug))
            debug[c] = true;
        break;
    } ARGEND

    if(*argv == '\0') {
        print("usage: %ca [-options] file.s\n", thechar);
        errorexit();
    }

    if(argc > 1) {
        nproc = 1;
        if(p = getenv("NPROC"))
            nproc = atol(p);
        c = 0;
        nout = 0;
        for(;;) {
            while(nout < nproc && argc > 0) {
                i = fork();
                if(i < 0) {
                    i = mywait(&status);
                    if(i < 0)
                        errorexit();
                    if(status)
                        c++;
                    nout--;
                    continue;
                }
                if(i == 0) {
                    print("%s:\n", *argv);
                    if(assemble(*argv))
                        errorexit();
                    exits(nil);
                }
                nout++;
                argc--;
                argv++;
            }
            i = mywait(&status);
            if(i < 0) {
                if(c)
                    errorexit();
                exits(nil);
            }
            if(status)
                c++;
            nout--;
        }
    }

    err = assemble(argv[0]);
    if(err > 0)
        errorexit();
    else
        exits(nil);
}

errorn
assemble(char *infile)
{
    fdt of;
    char ofile[100];
    char *p;
    char incfile[20];
    int i;

    strcpy(ofile, infile);
    p = utfrrune(ofile, '/');
    if(p) {
        *p++ = '\0';
        include[0] = ofile;
    } else
        p = ofile;
    if(outfile == nil) {
        outfile = p;
        if(outfile){
            p = utfrrune(outfile, '.');
            if(p)
                if(p[1] == 's' && p[2] == '\0')
                    p[0] = '\0';
            p = utfrune(outfile, '\0');
            p[0] = '.';
            p[1] = thechar;
            p[2] = '\0';
        } else
            outfile = "/dev/null";
    }
    p = getenv("INCLUDE");
    if(p) {
        setinclude(p);
    } else {
         sprint(incfile,"/%s/include", thestring);
         setinclude(strdup(incfile));
    }

    of = mycreat(outfile, 0664);
    if(of < 0) {
        yyerror("%ca: cannot create %s", thechar, outfile);
        errorexit();
    }
    Binit(&obuf, of, OWRITE);

    pass = 1;
    pinit(infile);
    for(i=0; i<nDlist; i++)
            dodefine(Dlist[i]);
    yyparse();
    if(nerrors) {
        cclean();
        return nerrors;
    }

    pass = 2;
    outhist();
    pinit(infile);
    for(i=0; i<nDlist; i++)
            dodefine(Dlist[i]);
    yyparse();

    cclean();
    return nerrors;
}

void
cclean(void)
{
    outcode(AEND, &nullgen, NOREG, &nullgen);
    Bflush(&obuf);
}
/*e: ea/main.c */
