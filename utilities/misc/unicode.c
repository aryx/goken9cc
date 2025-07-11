/*s: misc/unicode.c */
/*s: plan9 includes */
#include <u.h>
#include <libc.h>
/*e: plan9 includes */
#include <bio.h>

char    usage[] = "unicode { [-t] hex hex ... | hexmin-hexmax ... | [-n] char ... }";
char    hex[] = "0123456789abcdefABCDEF";
int numout = 0;
int text = 0;
char    *err;
Biobuf  bout;

char    *range(char*[]);
char    *nums(char*[]);
char    *chars(char*[]);

void
main(int argc, char *argv[])
{
    ARGBEGIN{
    case 'n':
        numout = 1;
        break;
    case 't':
        text = 1;
        break;
    }ARGEND
    Binit(&bout, 1, OWRITE);
    if(argc == 0){
        fprint(2, "usage: %s\n", usage);
        exits("usage");
    }
    if(!numout && utfrune(argv[0], '-'))
        exits(range(argv));
    if(numout || strchr(hex, argv[0][0])==0)
        exits(nums(argv));
    exits(chars(argv));
}

char*
range(char *argv[])
{
    char *q;
    int min, max;
    int i;

    while(*argv){
        q = *argv;
        if(strchr(hex, q[0]) == 0){
    err:
            fprint(2, "unicode: bad range %s\n", *argv);
            return "bad range";
        }
        min = strtoul(q, &q, 16);
        if(min<0 || min>Runemax || *q!='-')
            goto err;
        q++;
        if(strchr(hex, *q) == 0)
            goto err;
        max = strtoul(q, &q, 16);
        if(max<0 || max>Runemax || max<min || *q!=0)
            goto err;
        i = 0;
        do{
            Bprint(&bout, "%.6x %C", min, min);
            i++;
            if(min==max || (i&7)==0)
                Bprint(&bout, "\n");
            else
                Bprint(&bout, "\t");
            min++;
        }while(min<=max);
        argv++;
    }
    return 0;
}

char*
nums(char *argv[])
{
    char *q;
    Rune r;
    int w, rsz;
    char utferr[UTFmax];

    r = Runeerror;
    rsz = runetochar(utferr, &r);
    while(*argv){
        q = *argv;
        while(*q){
            w = chartorune(&r, q);
            if(r==Runeerror){
                if(strlen(q) != rsz || memcmp(q, utferr, rsz) != 0){
                    fprint(2, "unicode: invalid utf string %s\n", *argv);
                    return "bad utf";
                }
            }
            Bprint(&bout, "%.6x\n", r);
            q += w;
        }
        argv++;
    }
    return 0;
}

char*
chars(char *argv[])
{
    char *q;
    int m;

    while(*argv){
        q = *argv;
        if(strchr(hex, q[0]) == 0){
    err:
            fprint(2, "unicode: bad unicode value %s\n", *argv);
            return "bad char";
        }
        m = strtoul(q, &q, 16);
        if(m<0 || m>Runemax || *q!=0)
            goto err;
        Bprint(&bout, "%C", m);
        if(!text)
            Bprint(&bout, "\n");
        argv++;
    }
    return 0;
}
/*e: misc/unicode.c */
