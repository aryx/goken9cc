/*s: generators/yacc/yacc.c */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

/*s: constant Bungetrune */
#define	Bungetrune	Bungetc		/* ok for now. */
/*e: constant Bungetrune */

/*s: constant TBITSET */
/*
 * all these are 32 bit
 */
#define TBITSET		((32+NTERMS)/32)	/* BOTCH?? +31 */
/*e: constant TBITSET */
/*s: function BIT */
#define BIT(a,i)	((a)[(i)>>5] & (1<<((i)&037)))
/*e: function BIT */
/*s: function SETBIT */
#define SETBIT(a,i)	((a)[(i)>>5] |= (1<<((i)&037)))
/*e: function SETBIT */
/*s: function NWORDS */
#define NWORDS(n)	(((n)+32)/32)
/*e: function NWORDS */

/*s: constant PARSER */
// was /sys/lib/yaccpar
#define PARSER		"#9/etc/yaccpar"
/*e: constant PARSER */
/*s: constant PARSERS */
// was /sys/lib/yaccpars
#define PARSERS		"#9/etc/yaccpars"
/*e: constant PARSERS */
/*s: constant TEMPNAME */
#define TEMPNAME	"y.tmp.XXXXXX"
/*e: constant TEMPNAME */
/*s: constant ACTNAME */
#define ACTNAME		"y.acts.XXXXXX"
/*e: constant ACTNAME */
/*s: constant OFILE */
#define OFILE		"tab.c"
/*e: constant OFILE */
/*s: constant FILEU */
#define FILEU		"output"
/*e: constant FILEU */
/*s: constant FILED */
#define FILED		"tab.h"
/*e: constant FILED */
/*s: constant FILEDEBUG */
#define FILEDEBUG	"debug"
/*e: constant FILEDEBUG */

/*s: enum _anon_ */
enum
{
/*
 * the following are adjustable
 * according to memory size
 */
    ACTSIZE		= 40000,
    MEMSIZE		= 40000,
    NSTATES		= 2000,
    NTERMS		= 511,
    NPROD		= 1600,
    NNONTERM	= 600,
    TEMPSIZE	= 2000,
    CNAMSZ		= 10000,
    LSETSIZE	= 2400,
    WSETSIZE	= 350,

    NAMESIZE	= 50,
    NTYPES		= 63,
    ISIZE		= 400,

    PRIVATE		= 0xE000,	/* unicode private use */

    /* relationships which must hold:
        TBITSET ints must hold NTERMS+1 bits...
        WSETSIZE >= NNONTERM
        LSETSIZE >= NNONTERM
        TEMPSIZE >= NTERMS + NNONTERM + 1
        TEMPSIZE >= NSTATES
    */

    NTBASE		= 010000,
    ERRCODE		= 8190,
    ACCEPTCODE	= 8191,

    NOASC		= 0,	/* no assoc. */
    LASC		= 1,	/* left assoc. */
    RASC		= 2,	/* right assoc. */
    BASC		= 3,	/* binary assoc. */

    /* flags for state generation */

    DONE		= 0,
    MUSTDO		= 1,
    MUSTLOOKAHEAD	= 2,

    /* flags for a rule having an action, and being reduced */

    ACTFLAG		= 04,
    REDFLAG		= 010,

    /* output parser flags */
    YYFLAG1		= -1000,

    /* parse tokens */
    IDENTIFIER	= PRIVATE,
    MARK,
    TERM,
    LEFT,
    RIGHT,
    BINARY,
    PREC,
    LCURLY,
    IDENTCOLON,
    NUMBER,
    START,
    TYPEDEF,
    TYPENAME,
    UNION,

    ENDFILE		= 0,

    EMPTY		= 1,
    WHOKNOWS	= 0,
    OK		= 1,
    NOMORE		= -1000,
};
/*e: enum _anon_ */

    /* command to clobber tempfiles after use */

#define	ZAPFILE(x)	if(x) remove(x)


    /* macros for getting associativity and precedence levels */

/*s: function ASSOC */
#define ASSOC(i)	((i)&03)
/*e: function ASSOC */
/*s: function PLEVEL */
#define PLEVEL(i)	(((i)>>4)&077)
/*e: function PLEVEL */
/*s: function TYPE */
#define TYPE(i)		(((i)>>10)&077)
/*e: function TYPE */

    /* macros for setting associativity and precedence levels */

/*s: function SETASC */
#define SETASC(i,j)	i |= j
/*e: function SETASC */
/*s: function SETPLEV */
#define SETPLEV(i,j)	i |= (j<<4)
/*e: function SETPLEV */
/*s: function SETTYPE */
#define SETTYPE(i,j)	i |= (j<<10)
/*e: function SETTYPE */

    /* looping macros */

#define TLOOP(i)	for(i=1; i<=ntokens; i++)
#define NTLOOP(i)	for(i=0; i<=nnonter; i++)
#define PLOOP(s,i)	for(i=s; i<nprod; i++)
#define SLOOP(i)	for(i=0; i<nstate; i++)
/*s: function WSBUMP */
#define WSBUMP(x)	x++
/*e: function WSBUMP */
#define WSLOOP(s,j)	for(j=s; j<cwp; j++)
#define ITMLOOP(i,p,q)	for(q=pstate[i+1], p=pstate[i]; p<q; p++)
#define SETLOOP(i)	for(i=0; i<tbitset; i++)

    /* I/O descriptors */

/*s: global faction */
Biobuf*	faction;	/* file for saving actions */
/*e: global faction */
/*s: global fdefine */
Biobuf*	fdefine;	/* file for #defines */
/*e: global fdefine */
/*s: global fdebug */
Biobuf*	fdebug;		/* y.debug for strings for debugging */
/*e: global fdebug */
/*s: global ftable */
Biobuf*	ftable;		/* y.tab.c file */
/*e: global ftable */
/*s: global ftemp */
Biobuf*	ftemp;		/* tempfile to pass 2 */
/*e: global ftemp */
/*s: global finput */
Biobuf*	finput;		/* input file */
/*e: global finput */
/*s: global foutput */
Biobuf*	foutput;	/* y.output file */
/*e: global foutput */

    /* communication variables between various I/O routines */

/*s: global infile */
char*	infile;			/* input file name */
/*e: global infile */
/*s: global numbval */
int	numbval;		/* value of an input number */
/*e: global numbval */
/*s: global tokname */
char	tokname[NAMESIZE+UTFmax+1]; /* input token name, slop for runes and 0 */
/*e: global tokname */

    /* structure declarations */

typedef
struct
{
    int	lset[TBITSET];
} Lkset;

typedef
struct
{
    int*	pitem;
    Lkset*	look;
} Item;

typedef
struct
{
    char*	name;
    int	value;
} Symb;

typedef
struct
{
    int*	pitem;
    int	flag;
    Lkset	ws;
} Wset;

    /* storage of names */

/*s: global cnames */
char	cnames[CNAMSZ];		/* place where token and nonterminal names are stored */
/*e: global cnames */
/*s: global cnamsz */
int	cnamsz = CNAMSZ;	/* size of cnames */
/*e: global cnamsz */
/*s: global cnamp */
char*	cnamp = cnames;		/* place where next name is to be put in */
/*e: global cnamp */
/*s: global ndefout */
int	ndefout = 4;		/* number of defined symbols output */
/*e: global ndefout */
/*s: global tempname */
char*	tempname;
/*e: global tempname */
/*s: global actname */
char*	actname;
/*e: global actname */
/*s: global ttempname */
char	ttempname[] = TEMPNAME;
/*e: global ttempname */
/*s: global tactname */
char	tactname[] = ACTNAME;
/*e: global tactname */
/*s: global parser */
char*	parser;// = PARSER;
/*e: global parser */
/*s: global yydebug */
char*	yydebug;
/*e: global yydebug */

    /* storage of types */
/*s: global ntypes */
int	ntypes;			/* number of types defined */
/*e: global ntypes */
/*s: global typeset */
char*	typeset[NTYPES];	/* pointers to type tags */
/*e: global typeset */

    /* token information */

/*s: global ntokens */
int	ntokens = 0 ;		/* number of tokens */
/*e: global ntokens */
/*s: global tokset */
Symb	tokset[NTERMS];
/*e: global tokset */
/*s: global toklev */
int	toklev[NTERMS];		/* vector with the precedence of the terminals */
/*e: global toklev */

    /* nonterminal information */

/*s: global nnonter */
int	nnonter = -1;		/* the number of nonterminals */
/*e: global nnonter */
/*s: global nontrst */
Symb	nontrst[NNONTERM];
/*e: global nontrst */
/*s: global start */
int	start;			/* start symbol */
/*e: global start */

    /* assigned token type values */
/*s: global extval */
int	extval = 0;
/*e: global extval */

/*s: global ytabc */
char*	ytabc = OFILE;	/* name of y.tab.c */
/*e: global ytabc */

    /* grammar rule information */

/*s: global mem0 */
int	mem0[MEMSIZE] ;		/* production storage */
/*e: global mem0 */
/*s: global mem */
int*	mem = mem0;
/*e: global mem */
/*s: global nprod */
int	nprod = 1;		/* number of productions */
/*e: global nprod */
/*s: global prdptr */
int*	prdptr[NPROD];		/* pointers to descriptions of productions */
/*e: global prdptr */
/*s: global levprd */
int	levprd[NPROD];		/* precedence levels for the productions */
/*e: global levprd */
/*s: global rlines */
int	rlines[NPROD];		/* line number for this rule */
/*e: global rlines */

    /* state information */

/*s: global nstate */
int	nstate = 0;		/* number of states */
/*e: global nstate */
/*s: global pstate */
Item*	pstate[NSTATES+2];	/* pointers to the descriptions of the states */
/*e: global pstate */
/*s: global tystate */
int	tystate[NSTATES];	/* contains type information about the states */
/*e: global tystate */
/*s: global defact */
int	defact[NSTATES];	/* the default actions of states */
/*e: global defact */
/*s: global tstates */
int	tstates[NTERMS];	/* states generated by terminal gotos */
/*e: global tstates */
/*s: global ntstates */
int	ntstates[NNONTERM]; 	/* states generated by nonterminal gotos */
/*e: global ntstates */
/*s: global mstates */
int	mstates[NSTATES];	/* chain of overflows of term/nonterm generation lists  */
/*e: global mstates */
/*s: global lastred */
int	lastred; 		/* the number of the last reduction of a state */
/*e: global lastred */

    /* lookahead set information */

/*s: global lkst */
Lkset	lkst[LSETSIZE];
/*e: global lkst */
/*s: global nolook */
int	nolook = 0;			/* flag to turn off lookahead computations */
/*e: global nolook */
/*s: global tbitset */
int	tbitset;		/* size of lookahead sets */
/*e: global tbitset */
/*s: global nlset */
int	nlset = 0;		/* next lookahead set index */
/*e: global nlset */
/*s: global clset */
Lkset	clset;  		/* temporary storage for lookahead computations */
/*e: global clset */

    /* working set information */

/*s: global wsets */
Wset	wsets[WSETSIZE];
/*e: global wsets */
/*s: global cwp */
Wset*	cwp;
/*e: global cwp */

    /* storage for action table */

/*s: global amem */
int	amem[ACTSIZE];		/* action table storage */
/*e: global amem */
/*s: global memp */
int*	memp = amem;		/* next free action table position */
/*e: global memp */
/*s: global indgo */
int	indgo[NSTATES];		/* index to the stored goto table */
/*e: global indgo */

    /* temporary vector, indexable by states, terms, or ntokens */

/*s: global temp1 */
int	temp1[TEMPSIZE];	/* temporary storage, indexed by terms + ntokens or states */
/*e: global temp1 */
/*s: global lineno */
int	lineno = 1;		/* current input line number */
/*e: global lineno */
/*s: global fatfl */
int	fatfl = 1;  		/* if on, error is fatal */
/*e: global fatfl */
/*s: global nerrors */
int	nerrors = 0;		/* number of errors */
/*e: global nerrors */

    /* statistics collection variables */

/*s: global zzgoent */
int	zzgoent = 0;
/*e: global zzgoent */
/*s: global zzgobest */
int	zzgobest = 0;
/*e: global zzgobest */
/*s: global zzacent */
int	zzacent = 0;
/*e: global zzacent */
/*s: global zzexcp */
int	zzexcp = 0;
/*e: global zzexcp */
/*s: global zzclose */
int	zzclose = 0;
/*e: global zzclose */
/*s: global zzrrconf */
int	zzrrconf = 0;
/*e: global zzrrconf */
/*s: global zzsrconf */
int	zzsrconf;
/*e: global zzsrconf */

/*s: global ggreed */
int*	ggreed = lkst[0].lset;
/*e: global ggreed */
/*s: global pgo */
int*	pgo = wsets[0].ws.lset;
/*e: global pgo */
/*s: global yypgo */
int*	yypgo = &nontrst[0].value;
/*e: global yypgo */

/*s: global maxspr */
int	maxspr = 0;  		/* maximum spread of any entry */
/*e: global maxspr */
/*s: global maxoff */
int	maxoff = 0;  		/* maximum offset into a array */
/*e: global maxoff */
/*s: global pmem */
int*	pmem = mem0;
/*e: global pmem */
/*s: global maxa */
int*	maxa;
/*e: global maxa */
/*s: global nxdb */
int	nxdb = 0;
/*e: global nxdb */
/*s: global adb */
int	adb = 0;
/*e: global adb */


    /* storage for information about the nonterminals */

/*s: global pres (generators/yacc/yacc.c) */
static int**	pres[NNONTERM+2];  	/* vector of pointers to productions yielding each nonterminal */
/*e: global pres (generators/yacc/yacc.c) */
/*s: global pfirst */
Lkset*	pfirst[NNONTERM+2];	/* vector of pointers to first sets for each nonterminal */
/*e: global pfirst */
/*s: global pempty */
int	pempty[NNONTERM+1];	/* vector of nonterminals nontrivially deriving e */
/*e: global pempty */

    /* random stuff picked out from between functions */

/*s: global indebug */
int	indebug = 0;
/*e: global indebug */
/*s: global zzcwp */
Wset*	zzcwp = wsets;
/*e: global zzcwp */
/*s: global zzmemsz */
int*	zzmemsz = mem0;
/*e: global zzmemsz */
/*s: global pidebug */
int	pidebug = 0;		/* debugging flag for putitem */
/*e: global pidebug */
/*s: global gsdebug */
int	gsdebug = 0;
/*e: global gsdebug */
/*s: global cldebug */
int	cldebug = 0;		/* debugging flag for closure */
/*e: global cldebug */
/*s: global pkdebug */
int	pkdebug = 0;
/*e: global pkdebug */
/*s: global g2debug */
int	g2debug = 0;
/*e: global g2debug */

/*s: global resrv */
struct
{
    char*	name;
    long	value;
} resrv[] =
{
    "binary",	BINARY,
    "left",		LEFT,
    "nonassoc",	BINARY,
    "prec",		PREC,
    "right",	RIGHT,
    "start",	START,
    "term",		TERM,
    "token",	TERM,
    "type",		TYPEDEF,
    "union",	UNION,
    0,
};
/*e: global resrv */

    /* define functions */

void	main(int, char**);
void	others(void);
char*	chcopy(char*, char*);
char*	writem(int*);
char*	symnam(int);
void	summary(void);
void	error(char*, ...);
void	aryfil(int*, int, int);
int	setunion(int*, int*);
void	prlook(Lkset*);
void	cpres(void);
void	cpfir(void);
int	state(int);
void	putitem(int*, Lkset*);
void	cempty(void);
void	stagen(void);
void	closure(int);
Lkset*	flset(Lkset*);
void	cleantmp(void);
void	intr(void);
void	setup(int, char**);
void	finact(void);
int	defin(int, char*);
void	defout(int);
char*	cstash(char*);
long	gettok(void);
int	fdtype(int);
int	chfind(int, char*);
void	cpyunion(void);
void	cpycode(void);
int	skipcom(void);
void	cpyact(int);
void	openup(char*, int, int, int, char*);
void	output(void);
int	apack(int*, int);
void	go2out(void);
void	go2gen(int);
void	precftn(int, int, int);
void	wract(int);
void	wrstate(int);
void	warray(char*, int*, int);
void	hideprod(void);
void	callopt(void);
void	gin(int);
void	stin(int);
int	nxti(void);
void	osummary(void);
void	aoutput(void);
void	arout(char*, int*, int);
int	gtnm(void);

/*s: function main (generators/yacc/yacc.c) */
void
main(int argc, char *argv[])
{

    char *s;
    if((s = getenv("YACCPAR")) != 0)
  parser = s ;
    else
        parser = unsharp(PARSER);

    setup(argc, argv);	/* initialize and read productions */
    tbitset = NWORDS(ntokens);
    cpres();		/* make table of which productions yield a given nonterminal */
    cempty();		/* make a table of which nonterminals can match the empty string */
    cpfir();		/* make a table of firsts of nonterminals */
    stagen();		/* generate the states */
    output();		/* write the states and the tables */
    go2out();
    hideprod();
    summary();
    callopt();
    others();
    exits(0);
}
/*e: function main (generators/yacc/yacc.c) */

/*s: function others */
/*
 * put out other arrays, copy the parsers
 */
void
others(void)
{
    int c, i, j;

    finput = Bopen(parser, OREAD);
    if(finput == 0)
        error("cannot find parser %s", parser);
    warray("yyr1", levprd, nprod);
    aryfil(temp1, nprod, 0);
    PLOOP(1, i)
        temp1[i] = prdptr[i+1]-prdptr[i]-2;
    warray("yyr2", temp1, nprod);

    aryfil(temp1, nstate, -1000);
    TLOOP(i)
        for(j=tstates[i]; j!=0; j=mstates[j])
            temp1[j] = i;
    NTLOOP(i)
        for(j=ntstates[i]; j!=0; j=mstates[j])
            temp1[j] = -i;
    warray("yychk", temp1, nstate);
    warray("yydef", defact, nstate);

    /* put out token translation tables */
    /* table 1 has 0-256 */
    aryfil(temp1, 256, 0);
    c = 0;
    TLOOP(i) {
        j = tokset[i].value;
        if(j >= 0 && j < 256) {
            if(temp1[j]) {
                print("yacc bug -- cant have 2 different Ts with same value\n");
                print("	%s and %s\n", tokset[i].name, tokset[temp1[j]].name);
                nerrors++;
            }
            temp1[j] = i;
            if(j > c)
                c = j;
        }
    }
    warray("yytok1", temp1, c+1);

    /* table 2 has PRIVATE-PRIVATE+256 */
    aryfil(temp1, 256, 0);
    c = 0;
    TLOOP(i) {
        j = tokset[i].value - PRIVATE;
        if(j >= 0 && j < 256) {
            if(temp1[j]) {
                print("yacc bug -- cant have 2 different Ts with same value\n");
                print("	%s and %s\n", tokset[i].name, tokset[temp1[j]].name);
                nerrors++;
            }
            temp1[j] = i;
            if(j > c)
                c = j;
        }
    }
    warray("yytok2", temp1, c+1);

    /* table 3 has everything else */
    Bprint(ftable, "long	yytok3[] =\n{\n");
    c = 0;
    TLOOP(i) {
        j = tokset[i].value;
        if(j >= 0 && j < 256)
            continue;
        if(j >= PRIVATE && j < 256+PRIVATE)
            continue;

        Bprint(ftable, "%4d,%4d,", j, i);
        c++;
        if(c%5 == 0)
            Bprint(ftable, "\n");
    }
    Bprint(ftable, "%4d\n};\n", 0);

    /* copy parser text */
    while((c=Bgetrune(finput)) != Beof) {
        if(c == '$') {
            if((c = Bgetrune(finput)) != 'A')
                Bputrune(ftable, '$');
            else { /* copy actions */
                faction = Bopen(actname, OREAD);
                if(faction == 0)
                    error("cannot reopen action tempfile");
                while((c=Bgetrune(faction)) != Beof)
                    Bputrune(ftable, c);
                Bterm(faction);
                ZAPFILE(actname);
                c = Bgetrune(finput);
            }
        }
        Bputrune(ftable, c);
    }
    Bterm(ftable);
}
/*e: function others */

/*s: function chcopy */
/*
 * copies string q into p, returning next free char ptr
 */
char*
chcopy(char* p, char* q)
{
    int c;

    while(c = *q) {
        if(c == '"')
            *p++ = '\\';
        *p++ = c;
        q++;
    }
    *p = 0;
    return p;
}
/*e: function chcopy */

/*s: function writem */
/*
 * creates output string for item pointed to by pp
 */
char*
writem(int *pp)
{
    int i,*p;
    static char sarr[ISIZE];
    char* q;

    for(p=pp; *p>0; p++)
        ;
    p = prdptr[-*p];
    q = chcopy(sarr, nontrst[*p-NTBASE].name);
    q = chcopy(q, ": ");
    for(;;) {
        *q = ' ';
        p++;
        if(p == pp)
            *q = '.';
        q++;
        *q = '\0';
        i = *p;
        if(i <= 0)
            break;
        q = chcopy(q, symnam(i));
        if(q > &sarr[ISIZE-30])
            error("item too big");
    }

    /* an item calling for a reduction */
    i = *pp;
    if(i < 0 ) {
        q = chcopy(q, "    (");
        sprint(q, "%d)", -i);
    }
    return sarr;
}
/*e: function writem */

/*s: function symnam */
/*
 * return a pointer to the name of symbol i
 */
char*
symnam(int i)
{
    char* cp;

    cp = (i >= NTBASE)? nontrst[i-NTBASE].name: tokset[i].name;
    if(*cp == ' ')
        cp++;
    return cp;
}
/*e: function symnam */

/*s: function summary */
/*
 * output the summary on y.output
 */
void
summary(void)
{

    if(foutput != 0) {
        Bprint(foutput, "\n%d/%d terminals, %d/%d nonterminals\n",
            ntokens, NTERMS, nnonter, NNONTERM);
        Bprint(foutput, "%d/%d grammar rules, %d/%d states\n",
            nprod, NPROD, nstate, NSTATES);
        Bprint(foutput, "%d shift/reduce, %d reduce/reduce conflicts reported\n",
            zzsrconf, zzrrconf);
        Bprint(foutput, "%d/%d working sets used\n",
            (int)(zzcwp-wsets), WSETSIZE);
        Bprint(foutput, "memory: states,etc. %d/%d, parser %d/%d\n",
            (int)(zzmemsz-mem0), MEMSIZE, (int)(memp-amem), ACTSIZE);
        Bprint(foutput, "%d/%d distinct lookahead sets\n", nlset, LSETSIZE);
        Bprint(foutput, "%d extra closures\n", zzclose - 2*nstate);
        Bprint(foutput, "%d shift entries, %d exceptions\n", zzacent, zzexcp);
        Bprint(foutput, "%d goto entries\n", zzgoent);
        Bprint(foutput, "%d entries saved by goto default\n", zzgobest);
    }
    if(zzsrconf != 0 || zzrrconf != 0) {
        print("\nconflicts: ");
        if(zzsrconf)
            print("%d shift/reduce", zzsrconf);
        if(zzsrconf && zzrrconf)
            print(", ");
        if(zzrrconf)
            print("%d reduce/reduce", zzrrconf);
        print("\n");
    }
    if(ftemp != 0) {
        Bterm(ftemp);
        ftemp = 0;
    }
    if(fdefine != 0) {
        Bterm(fdefine);
        fdefine = 0;
    }
}
/*e: function summary */

/*s: function error (generators/yacc/yacc.c) */
/*
 * write out error comment -- NEEDS WORK
 */
void
error(char *s, ...)
{
 va_list arg;

    nerrors++;
    fprint(2, "\n fatal error:");
    // THIS does not work, need to use va_start probably
    //old: fprint(2, s, (&s)[1]);
 va_start(arg, s);
 vfprint(2, s, arg);
 va_end(arg);
    fprint(2, ", %s:%d\n", infile, lineno);
    if(!fatfl)
        return;
    summary();
    cleantmp();
    exits("error");
}
/*e: function error (generators/yacc/yacc.c) */

/*s: function aryfil */
/*
 * set elements 0 through n-1 to c
 */
void
aryfil(int *v, int n, int c)
{
    int i;

    for(i=0; i<n; i++)
        v[i] = c;
}
/*e: function aryfil */

/*s: function setunion */
/*
 * set a to the union of a and b
 * return 1 if b is not a subset of a, 0 otherwise
 */
int
setunion(int *a, int *b)
{
    int i, x, sub;

    sub = 0;
    SETLOOP(i) {
        x = *a;
        *a |= *b;
        if(*a != x)
            sub = 1;
        a++;
        b++;
    }
    return sub;
}
/*e: function setunion */

/*s: function prlook */
void
prlook(Lkset* p)
{
    int j, *pp;

    pp = p->lset;
    if(pp == 0)
        Bprint(foutput, "\tNULL");
    else {
        Bprint(foutput, " { ");
        TLOOP(j)
            if(BIT(pp,j))
                Bprint(foutput, "%s ", symnam(j));
        Bprint(foutput, "}");
    }
}
/*e: function prlook */

/*s: function cpres */
/*
 * compute an array with the beginnings of  productions yielding given nonterminals
 * The array pres points to these lists
 * the array pyield has the lists: the total size is only NPROD+1
 */
void
cpres(void)
{
    int c, j, i, **pmem;
    static int *pyield[NPROD];

    pmem = pyield;
    NTLOOP(i) {
        c = i+NTBASE;
        pres[i] = pmem;
        fatfl = 0;  	/* make undefined  symbols  nonfatal */
        PLOOP(0, j)
            if(*prdptr[j] == c)
                *pmem++ =  prdptr[j]+1;
        if(pres[i] == pmem)
            error("nonterminal %s not defined!", nontrst[i].name);
    }
    pres[i] = pmem;
    fatfl = 1;
    if(nerrors) {
        summary();
        cleantmp();
        exits("error");
    }
    if(pmem != &pyield[nprod])
        error("internal Yacc error: pyield %d", pmem-&pyield[nprod]);
}
/*e: function cpres */

/*s: function cpfir */
/*
 * compute an array with the first of nonterminals
 */
void
cpfir(void)
{
    int *p, **s, i, **t, ch, changes;

    zzcwp = &wsets[nnonter];
    NTLOOP(i) {
        aryfil(wsets[i].ws.lset, tbitset, 0);
        t = pres[i+1];
        /* initially fill the sets */
        for(s=pres[i]; s<t; ++s)
            for(p = *s; (ch = *p) > 0; ++p) {
                if(ch < NTBASE) {
                    SETBIT(wsets[i].ws.lset, ch);
                    break;
                }
                if(!pempty[ch-NTBASE])
                    break;
            }
    }

    /* now, reflect transitivity */
    changes = 1;
    while(changes) {
        changes = 0;
        NTLOOP(i) {
            t = pres[i+1];
            for(s = pres[i]; s < t; ++s)
                for(p = *s; (ch = (*p-NTBASE)) >= 0; ++p) {
                    changes |= setunion(wsets[i].ws.lset, wsets[ch].ws.lset);
                    if(!pempty[ch])
                        break;
                }
        }
    }

    NTLOOP(i)
        pfirst[i] = flset(&wsets[i].ws);
    if(!indebug)
        return;
    if(foutput != 0)
        NTLOOP(i) {
            Bprint(foutput, "\n%s: ", nontrst[i].name);
            prlook(pfirst[i]);
            Bprint(foutput, " %d\n", pempty[i]);
        }
}
/*e: function cpfir */

/*s: function state */
/*
 * sorts last state,and sees if it equals earlier ones. returns state number
 */
int
state(int c)
{
    Item *p1, *p2, *k, *l, *q1, *q2;
    int size1, size2, i;

    p1 = pstate[nstate];
    p2 = pstate[nstate+1];
    if(p1 == p2)
        return 0;	/* null state */
    /* sort the items */
    for(k = p2-1; k > p1; k--)	/* make k the biggest */
        for(l = k-1; l >= p1; --l)
            if(l->pitem > k->pitem) {
                int *s;
                Lkset *ss;

                s = k->pitem;
                k->pitem = l->pitem;
                l->pitem = s;
                ss = k->look;
                k->look = l->look;
                l->look = ss;
            }
    size1 = p2 - p1;	/* size of state */

    for(i = (c>=NTBASE)? ntstates[c-NTBASE]: tstates[c]; i != 0; i = mstates[i]) {
        /* get ith state */
        q1 = pstate[i];
        q2 = pstate[i+1];
        size2 = q2 - q1;
        if(size1 != size2)
            continue;
        k = p1;
        for(l = q1; l < q2; l++) {
            if(l->pitem != k->pitem)
                break;
            k++;
        }
        if(l != q2)
            continue;
        /* found it */
        pstate[nstate+1] = pstate[nstate];	/* delete last state */
        /* fix up lookaheads */
        if(nolook)
            return i;
        for(l = q1, k = p1; l < q2; ++l, ++k ) {
            int s;

            SETLOOP(s)
                clset.lset[s] = l->look->lset[s];
            if(setunion(clset.lset, k->look->lset)) {
                tystate[i] = MUSTDO;
                /* register the new set */
                l->look = flset( &clset );
            }
        }
        return i;
    }
    /* state is new */
    if(nolook)
        error("yacc state/nolook error");
    pstate[nstate+2] = p2;
    if(nstate+1 >= NSTATES)
        error("too many states");
    if(c >= NTBASE) {
        mstates[nstate] = ntstates[c-NTBASE];
        ntstates[c-NTBASE] = nstate;
    } else {
        mstates[nstate] = tstates[c];
        tstates[c] = nstate;
    }
    tystate[nstate] = MUSTDO;
    return nstate++;
}
/*e: function state */

/*s: function putitem */
void
putitem(int *ptr, Lkset *lptr)
{
    Item *j;

    if(pidebug && foutput != 0)
        Bprint(foutput, "putitem(%s), state %d\n", writem(ptr), nstate);
    j = pstate[nstate+1];
    j->pitem = ptr;
    if(!nolook)
        j->look = flset(lptr);
    pstate[nstate+1] = ++j;
    if((int*)j > zzmemsz) {
        zzmemsz = (int*)j;
        if(zzmemsz >=  &mem0[MEMSIZE])
            error("out of state space");
    }
}
/*e: function putitem */

/*s: function cempty */
/*
 * mark nonterminals which derive the empty string
 * also, look for nonterminals which don't derive any token strings
 */
void
cempty(void)
{

    int i, *p;

    /* first, use the array pempty to detect productions that can never be reduced */
    /* set pempty to WHONOWS */
    aryfil(pempty, nnonter+1, WHOKNOWS);

    /* now, look at productions, marking nonterminals which derive something */
more:
    PLOOP(0, i) {
        if(pempty[*prdptr[i] - NTBASE])
            continue;
        for(p = prdptr[i]+1; *p >= 0; ++p)
            if(*p >= NTBASE && pempty[*p-NTBASE] == WHOKNOWS)
                break;
        /* production can be derived */
        if(*p < 0) {
            pempty[*prdptr[i]-NTBASE] = OK;
            goto more;
        }
    }

    /* now, look at the nonterminals, to see if they are all OK */
    NTLOOP(i) {
        /* the added production rises or falls as the start symbol ... */
        if(i == 0)
            continue;
        if(pempty[i] != OK) {
            fatfl = 0;
            error("nonterminal %s never derives any token string", nontrst[i].name);
        }
    }

    if(nerrors) {
        summary();
        cleantmp();
        exits("error");
    }

    /* now, compute the pempty array, to see which nonterminals derive the empty string */
    /* set pempty to WHOKNOWS */
    aryfil( pempty, nnonter+1, WHOKNOWS);

    /* loop as long as we keep finding empty nonterminals */

again:
    PLOOP(1, i) {
        /* not known to be empty */
        if(pempty[*prdptr[i]-NTBASE] == WHOKNOWS) {
            for(p = prdptr[i]+1; *p >= NTBASE && pempty[*p-NTBASE] == EMPTY ; ++p)
                ;
            /* we have a nontrivially empty nonterminal */
            if(*p < 0) {
                pempty[*prdptr[i]-NTBASE] = EMPTY;
                /* got one ... try for another */
                goto again;
            }
        }
    }
}
/*e: function cempty */

/*s: function stagen */
/*
 * generate the states
 */
void
stagen(void)
{

    int c, i, j, more;
    Wset *p, *q;

    /* initialize */
    nstate = 0;

    /* THIS IS FUNNY from the standpoint of portability
     * it represents the magic moment when the mem0 array, which has
     * been holding the productions, starts to hold item pointers, of a
     * different type...
     * someday, alloc should be used to allocate all this stuff... for now, we
     * accept that if pointers don't fit in integers, there is a problem...
     */

    pstate[0] = pstate[1] = (Item*)mem;
    aryfil(clset.lset, tbitset, 0);
    putitem(prdptr[0]+1, &clset);
    tystate[0] = MUSTDO;
    nstate = 1;
    pstate[2] = pstate[1];

    aryfil(amem, ACTSIZE, 0);

    /* now, the main state generation loop */
    for(more=1; more;) {
        more = 0;
        SLOOP(i) {
            if(tystate[i] != MUSTDO)
                continue;
            tystate[i] = DONE;
            aryfil(temp1, nnonter+1, 0);
            /* take state i, close it, and do gotos */
            closure(i);
            /* generate goto's */
            WSLOOP(wsets, p) {
                if(p->flag)
                    continue;
                p->flag = 1;
                c = *(p->pitem);
                if(c <= 1) {
                    if(pstate[i+1]-pstate[i] <= p-wsets)
                        tystate[i] = MUSTLOOKAHEAD;
                    continue;
                }
                /* do a goto on c */
                WSLOOP(p, q)
                    /* this item contributes to the goto */
                    if(c == *(q->pitem)) {
                        putitem(q->pitem+1, &q->ws);
                        q->flag = 1;
                    }
                if(c < NTBASE)
                    state(c);	/* register new state */
                else
                    temp1[c-NTBASE] = state(c);
            }
            if(gsdebug && foutput != 0) {
                Bprint(foutput, "%d: ", i);
                NTLOOP(j)
                    if(temp1[j])
                        Bprint(foutput, "%s %d, ",
                        nontrst[j].name, temp1[j]);
                Bprint(foutput, "\n");
            }
            indgo[i] = apack(&temp1[1], nnonter-1) - 1;
            /* do some more */
            more = 1;
        }
    }
}
/*e: function stagen */

/*s: function closure */
/*
 * generate the closure of state i
 */
void
closure(int i)
{

    Wset *u, *v;
    Item *p, *q;
    int c, ch, work, k, *pi, **s, **t;

    zzclose++;

    /* first, copy kernel of state i to wsets */
    cwp = wsets;
    ITMLOOP(i, p, q) {
        cwp->pitem = p->pitem;
        cwp->flag = 1;			/* this item must get closed */
        SETLOOP(k)
            cwp->ws.lset[k] = p->look->lset[k];
        WSBUMP(cwp);
    }

    /* now, go through the loop, closing each item */
    work = 1;
    while(work) {
        work = 0;
        WSLOOP(wsets, u) {
            if(u->flag == 0)
                continue;
            /* dot is before c */
            c = *(u->pitem);
            if(c < NTBASE) {
                u->flag = 0;
                /* only interesting case is where . is before nonterminal */
                continue;
            }

            /* compute the lookahead */
            aryfil(clset.lset, tbitset, 0);

            /* find items involving c */
            WSLOOP(u, v)
                if(v->flag == 1 && *(pi=v->pitem) == c) {
                    v->flag = 0;
                    if(nolook)
                        continue;
                    while((ch = *++pi) > 0) {
                        /* terminal symbol */
                        if(ch < NTBASE) {
                            SETBIT(clset.lset, ch);
                            break;
                        }
                        /* nonterminal symbol */
                        setunion(clset.lset, pfirst[ch-NTBASE]->lset);
                        if(!pempty[ch-NTBASE])
                            break;
                    }
                    if(ch <= 0)
                        setunion(clset.lset, v->ws.lset);
                }

            /*
             * now loop over productions derived from c
             * c is now nonterminal number
             */
            c -= NTBASE;
            t = pres[c+1];
            for(s = pres[c]; s < t; ++s) {
                /*
                 * put these items into the closure
                 * is the item there
                 */
                WSLOOP(wsets, v)
                    /* yes, it is there */
                    if(v->pitem == *s) {
                        if(nolook)
                            goto nexts;
                        if(setunion(v->ws.lset, clset.lset))
                            v->flag = work = 1;
                        goto nexts;
                    }

                /*  not there; make a new entry */
                if(cwp-wsets+1 >= WSETSIZE)
                    error( "working set overflow");
                cwp->pitem = *s;
                cwp->flag = 1;
                if(!nolook) {
                    work = 1;
                    SETLOOP(k) cwp->ws.lset[k] = clset.lset[k];
                }
                WSBUMP(cwp);

            nexts:;
            }
        }
    }

    /* have computed closure; flags are reset; return */
    if(cwp > zzcwp)
        zzcwp = cwp;
    if(cldebug && foutput != 0) {
        Bprint(foutput, "\nState %d, nolook = %d\n", i, nolook);
        WSLOOP(wsets, u) {
            if(u->flag)
                Bprint(foutput, "flag set!\n");
            u->flag = 0;
            Bprint(foutput, "\t%s", writem(u->pitem));
            prlook(&u->ws);
            Bprint(foutput, "\n");
        }
    }
}
/*e: function closure */

/*s: function flset */
/*
 * decide if the lookahead set pointed to by p is known
 * return pointer to a perminent location for the set
 */
Lkset*
flset(Lkset *p)
{
    Lkset *q;
    int *u, *v, *w, j;

    for(q = &lkst[nlset]; q-- > lkst;) {
        u = p->lset;
        v = q->lset;
        w = &v[tbitset];
        while(v < w)
            if(*u++ != *v++)
                goto more;
        /* we have matched */
        return q;
    more:;
    }
    /* add a new one */
    q = &lkst[nlset++];
    if(nlset >= LSETSIZE)
        error("too many lookahead sets");
    SETLOOP(j)
        q->lset[j] = p->lset[j];
    return q;
}
/*e: function flset */

/*s: function cleantmp */
void
cleantmp(void)
{
    ZAPFILE(actname);
    ZAPFILE(tempname);
}
/*e: function cleantmp */

/*s: function intr */
void
intr(void)
{
    cleantmp();
    exits("interrupted");
}
/*e: function intr */

/*s: function usage */
void
usage(void)
{
    fprint(2, "usage: yacc [-Dn] [-vdS] [-o outputfile] [-s stem] grammar\n");
    exits("usage");
}
/*e: function usage */

/*s: function setup */
void
setup(int argc, char *argv[])
{
    long c, t;
    int i, j, lev, ty, ytab, *p;
    int vflag, dflag, stem;
    char actnm[8], *stemc, *s, dirbuf[128];

    ytab = 0;
    vflag = 0;
    dflag = 0;
    stem = 0;
    stemc = "y";
    foutput = 0;
    fdefine = 0;
    fdebug = 0;
    ARGBEGIN{
    case 'v':
    case 'V':
        vflag++;
        break;
    case 'D':
        yydebug = EARGF(usage());
        break;
    case 'd':
        dflag++;
        break;
    case 'o':
        ytab++;
        ytabc = EARGF(usage());
        break;
    case 's':
        stem++;
        stemc = ARGF();
        break;
    case 'S':
        parser = unsharp(PARSERS);
        break;
    default:
        error("illegal option: %c", ARGC());
    }ARGEND
    openup(stemc, dflag, vflag, ytab, ytabc);

    ftemp = Bopen(tempname = mktemp(ttempname), OWRITE);
    faction = Bopen(actname = mktemp(tactname), OWRITE);
    if(ftemp == 0 || faction == 0)
        error("cannot open temp file");
    if(argc < 1)
        error("no input file");
    infile = argv[0];
    if(infile[0] != '/' && getwd(dirbuf, sizeof dirbuf)!=nil){
        i = strlen(infile)+1+strlen(dirbuf)+1+10;
        s = malloc(i);
        if(s != nil){
            snprint(s, i, "%s/%s", dirbuf, infile);
            cleanname(s);
            infile = s;
        }
    }
    finput = Bopen(infile, OREAD);
    if(finput == 0)
        error("cannot open '%s'", argv[0]);
    cnamp = cnames;

    defin(0, "$end");
    extval = PRIVATE;	/* tokens start in unicode 'private use' */
    defin(0, "error");
    defin(1, "$accept");
    defin(0, "$unk");
    mem = mem0;
    i = 0;

    for(t = gettok(); t != MARK && t != ENDFILE;)
    switch(t) {
    case ';':
        t = gettok();
        break;

    case START:
        if(gettok() != IDENTIFIER)
            error("bad %%start construction");
        start = chfind(1, tokname);
        t = gettok();
        continue;

    case TYPEDEF:
        if(gettok() != TYPENAME)
            error("bad syntax in %%type");
        ty = numbval;
        for(;;) {
            t = gettok();
            switch(t) {
            case IDENTIFIER:
                if((t=chfind(1, tokname)) < NTBASE) {
                    j = TYPE(toklev[t]);
                    if(j != 0 && j != ty)
                        error("type redeclaration of token %s",
                            tokset[t].name);
                    else
                        SETTYPE(toklev[t], ty);
                } else {
                    j = nontrst[t-NTBASE].value;
                    if(j != 0 && j != ty)
                        error("type redeclaration of nonterminal %s",
                            nontrst[t-NTBASE].name );
                    else
                        nontrst[t-NTBASE].value = ty;
                }
            case ',':
                continue;
            case ';':
                t = gettok();
            default:
                break;
            }
            break;
        }
        continue;

    case UNION:
        /* copy the union declaration to the output */
        cpyunion();
        t = gettok();
        continue;

    case LEFT:
    case BINARY:
    case RIGHT:
        i++;

    case TERM:
        /* nonzero means new prec. and assoc. */
        lev = t-TERM;
        ty = 0;

        /* get identifiers so defined */
        t = gettok();

        /* there is a type defined */
        if(t == TYPENAME) {
            ty = numbval;
            t = gettok();
        }
        for(;;) {
            switch(t) {
            case ',':
                t = gettok();
                continue;

            case ';':
                break;

            case IDENTIFIER:
                j = chfind(0, tokname);
                if(j >= NTBASE)
                    error("%s defined earlier as nonterminal", tokname);
                if(lev) {
                    if(ASSOC(toklev[j]))
                        error("redeclaration of precedence of %s", tokname);
                    SETASC(toklev[j], lev);
                    SETPLEV(toklev[j], i);
                }
                if(ty) {
                    if(TYPE(toklev[j]))
                        error("redeclaration of type of %s", tokname);
                    SETTYPE(toklev[j],ty);
                }
                t = gettok();
                if(t == NUMBER) {
                    tokset[j].value = numbval;
                    if(j < ndefout && j > 3)
                        error("please define type number of %s earlier",
                            tokset[j].name);
                    t = gettok();
                }
                continue;
            }
            break;
        }
        continue;

    case LCURLY:
        defout(0);
        cpycode();
        t = gettok();
        continue;

    default:
        error("syntax error");
    }
    if(t == ENDFILE)
        error("unexpected EOF before %%");

    /* t is MARK */
    Bprint(ftable, "extern	int	yyerrflag;\n");
    Bprint(ftable, "#ifndef	YYMAXDEPTH\n");
    Bprint(ftable, "#define	YYMAXDEPTH	150\n");
    Bprint(ftable, "#endif\n" );
    if(!ntypes) {
        Bprint(ftable, "#ifndef	YYSTYPE\n");
        Bprint(ftable, "#define	YYSTYPE	int\n");
        Bprint(ftable, "#endif\n");
    }
    Bprint(ftable, "YYSTYPE	yylval;\n");
    Bprint(ftable, "YYSTYPE	yyval;\n");

    prdptr[0] = mem;

    /* added production */
    *mem++ = NTBASE;

    /* if start is 0, we will overwrite with the lhs of the first rule */
    *mem++ = start;
    *mem++ = 1;
    *mem++ = 0;
    prdptr[1] = mem;
    while((t=gettok()) == LCURLY)
        cpycode();
    if(t != IDENTCOLON)
        error("bad syntax on first rule");

    if(!start)
        prdptr[0][1] = chfind(1, tokname);

    /* read rules */
    while(t != MARK && t != ENDFILE) {
        /* process a rule */
        rlines[nprod] = lineno;
        if(t == '|')
            *mem++ = *prdptr[nprod-1];
        else
            if(t == IDENTCOLON) {
                *mem = chfind(1, tokname);
                if(*mem < NTBASE)
                    error("token illegal on LHS of grammar rule");
                mem++;
            } else
                error("illegal rule: missing semicolon or | ?");
        /* read rule body */
        t = gettok();

    more_rule:
        while(t == IDENTIFIER) {
            *mem = chfind(1, tokname);
            if(*mem < NTBASE)
                levprd[nprod] = toklev[*mem];
            mem++;
            t = gettok();
        }
        if(t == PREC) {
            if(gettok() != IDENTIFIER)
                error("illegal %%prec syntax");
            j = chfind(2, tokname);
            if(j >= NTBASE)
                error("nonterminal %s illegal after %%prec",
                    nontrst[j-NTBASE].name);
            levprd[nprod] = toklev[j];
            t = gettok();
        }
        if(t == '=') {
            levprd[nprod] |= ACTFLAG;
            Bprint(faction, "\ncase %d:", nprod);
            cpyact(mem-prdptr[nprod]-1);
            Bprint(faction, " break;");
            if((t=gettok()) == IDENTIFIER) {

                /* action within rule... */
                sprint(actnm, "$$%d", nprod);

                /* make it a nonterminal */
                j = chfind(1, actnm);

                /*
                 * the current rule will become rule number nprod+1
                 * move the contents down, and make room for the null
                 */
                for(p = mem; p >= prdptr[nprod]; --p)
                    p[2] = *p;
                mem += 2;

                /* enter null production for action */
                p = prdptr[nprod];
                *p++ = j;
                *p++ = -nprod;

                /* update the production information */
                levprd[nprod+1] = levprd[nprod] & ~ACTFLAG;
                levprd[nprod] = ACTFLAG;
                if(++nprod >= NPROD)
                    error("more than %d rules", NPROD);
                prdptr[nprod] = p;

                /* make the action appear in the original rule */
                *mem++ = j;

                /* get some more of the rule */
                goto more_rule;
            }
        }

        while(t == ';')
            t = gettok();
        *mem++ = -nprod;

        /* check that default action is reasonable */
        if(ntypes && !(levprd[nprod]&ACTFLAG) && nontrst[*prdptr[nprod]-NTBASE].value) {

            /* no explicit action, LHS has value */
            int tempty;

            tempty = prdptr[nprod][1];
            if(tempty < 0)
                error("must return a value, since LHS has a type");
            else
                if(tempty >= NTBASE)
                    tempty = nontrst[tempty-NTBASE].value;
                else
                    tempty = TYPE(toklev[tempty]);
            if(tempty != nontrst[*prdptr[nprod]-NTBASE].value)
                error("default action causes potential type clash");
        }
        nprod++;
        if(nprod >= NPROD)
            error("more than %d rules", NPROD);
        prdptr[nprod] = mem;
        levprd[nprod] = 0;
    }

    /* end of all rules */
    defout(1);

    finact();
    if(t == MARK) {
        Bprint(ftable, "\n#line\t%d\t\"%s\"\n", lineno, infile);
        while((c=Bgetrune(finput)) != Beof)
            Bputrune(ftable, c);
    }
    Bterm(finput);
}
/*e: function setup */

/*s: function finact */
/*
 * finish action routine
 */
void
finact(void)
{

    Bterm(faction);
    Bprint(ftable, "#define YYEOFCODE %d\n", 1);
    Bprint(ftable, "#define YYERRCODE %d\n", 2);
}
/*e: function finact */

/*s: function defin */
/*
 * define s to be a terminal if t=0
 * or a nonterminal if t=1
 */
int
defin(int nt, char *s)
{
    int val;
    Rune rune;

    val = 0;
    if(nt) {
        nnonter++;
        if(nnonter >= NNONTERM)
            error("too many nonterminals, limit %d",NNONTERM);
        nontrst[nnonter].name = cstash(s);
        return NTBASE + nnonter;
    }

    /* must be a token */
    ntokens++;
    if(ntokens >= NTERMS)
        error("too many terminals, limit %d", NTERMS);
    tokset[ntokens].name = cstash(s);

    /* establish value for token */
    /* single character literal */
    if(s[0] == ' ') {
        val = chartorune(&rune, &s[1]);
        if(s[val+1] == 0) {
            val = rune;
            goto out;
        }
    }

    /* escape sequence */
    if(s[0] == ' ' && s[1] == '\\') {
        if(s[3] == 0) {
            /* single character escape sequence */
            switch(s[2]) {
            case 'n':	val = '\n'; break;
            case 'r':	val = '\r'; break;
            case 'b':	val = '\b'; break;
            case 't':	val = '\t'; break;
            case 'f':	val = '\f'; break;
            case '\'':	val = '\''; break;
            case '"':	val = '"'; break;
            case '\\':	val = '\\'; break;
            default:	error("invalid escape");
            }
            goto out;
        }

        /* \nnn sequence */
        if(s[2] >= '0' && s[2] <= '7') {
            if(s[3] < '0' ||
               s[3] > '7' ||
               s[4] < '0' ||
               s[4] > '7' ||
               s[5] != 0)
                error("illegal \\nnn construction");
            val = 64*s[2] + 8*s[3] + s[4] - 73*'0';
            if(val == 0)
                error("'\\000' is illegal");
            goto out;
        }
        error("unknown escape");
    }
    val = extval++;

out:
    tokset[ntokens].value = val;
    toklev[ntokens] = 0;
    return ntokens;
}
/*e: function defin */

/*s: function defout */
/*
 * write out the defines (at the end of the declaration section)
 */
void
defout(int last)
{
    int i, c;
    char sar[NAMESIZE+10];

    for(i=ndefout; i<=ntokens; i++) {
        /* non-literals */
        c = tokset[i].name[0];
        if(c != ' ' && c != '$') {
            Bprint(ftable, "#define	%s	%d\n",
                tokset[i].name, tokset[i].value);
            if(fdefine)
                Bprint(fdefine, "#define\t%s\t%d\n",
                    tokset[i].name, tokset[i].value);
        }
    }
    ndefout = ntokens+1;
    if(last && fdebug) {
        Bprint(fdebug, "char*	yytoknames[] =\n{\n");
        TLOOP(i) {
            if(tokset[i].name) {
                chcopy(sar, tokset[i].name);
                Bprint(fdebug, "\t\"%s\",\n", sar);
                continue;
            }
            Bprint(fdebug, "\t0,\n");
        }
        Bprint(fdebug, "};\n");
    }
}
/*e: function defout */

/*s: function cstash */
char*
cstash(char *s)
{
    char *temp;

    temp = cnamp;
    do {
        if(cnamp >= &cnames[cnamsz])
            error("too many characters in id's and literals");
        else
            *cnamp++ = *s;
    } while(*s++);
    return temp;
}
/*e: function cstash */

/*s: function gettok */
long
gettok(void)
{
    long c;
    Rune rune;
    int i, base, match, reserve;
    static int peekline;

begin:
    reserve = 0;
    lineno += peekline;
    peekline = 0;
    c = Bgetrune(finput);
    while(c == ' ' || c == '\n' || c == '\t' || c == '\f') {
        if(c == '\n')
            lineno++;
        c = Bgetrune(finput);
    }

    /* skip comment */
    if(c == '/') {
        lineno += skipcom();
        goto begin;
    }
    switch(c) {
    case Beof:
        return ENDFILE;

    case '{':
        Bungetrune(finput);
        return '=';

    case '<':
        /* get, and look up, a type name (union member name) */
        i = 0;
        while((c=Bgetrune(finput)) != '>' && c >= 0 && c != '\n') {
            rune = c;
            c = runetochar(&tokname[i], &rune);
            if(i < NAMESIZE)
                i += c;
        }
        if(c != '>')
            error("unterminated < ... > clause");
        tokname[i] = 0;
        for(i=1; i<=ntypes; i++)
            if(!strcmp(typeset[i], tokname)) {
                numbval = i;
                return TYPENAME;
            }
        ntypes++;
        numbval = ntypes;
        typeset[numbval] = cstash(tokname);
        return TYPENAME;

    case '"':
    case '\'':
        match = c;
        tokname[0] = ' ';
        i = 1;
        for(;;) {
            c = Bgetrune(finput);
            if(c == '\n' || c <= 0)
                error("illegal or missing ' or \"" );
            if(c == '\\') {
                tokname[i] = '\\';
                if(i < NAMESIZE)
                    i++;
                c = Bgetrune(finput);
            } else
                if(c == match)
                    break;
            rune = c;
            c = runetochar(&tokname[i], &rune);
            if(i < NAMESIZE)
                i += c;
        }
        break;

    case '%':
    case '\\':
        switch(c = Bgetrune(finput)) {
        case '0':	return TERM;
        case '<':	return LEFT;
        case '2':	return BINARY;
        case '>':	return RIGHT;
        case '%':
        case '\\':	return MARK;
        case '=':	return PREC;
        case '{':	return LCURLY;
        default:	reserve = 1;
        }

    default:
        /* number */
        if(isdigit(c)) {
            numbval = c-'0';
            base = (c=='0')? 8: 10;
            for(c = Bgetrune(finput); isdigit(c); c = Bgetrune(finput))
                numbval = numbval*base + (c-'0');
            Bungetrune(finput);
            return NUMBER;
        }
        if(islower(c) || isupper(c) || c=='_' || c=='.' || c=='$')  {
            i = 0;
            while(islower(c) || isupper(c) || isdigit(c) ||
                c == '-' || c=='_' || c=='.' || c=='$') {
                if(reserve && isupper(c))
                    c += 'a'-'A';
                rune = c;
                c = runetochar(&tokname[i], &rune);
                if(i < NAMESIZE)
                    i += c;
                c = Bgetrune(finput);
            }
        } else
            return c;
        Bungetrune(finput);
    }
    tokname[i] = 0;

    /* find a reserved word */
    if(reserve) {
        for(c=0; resrv[c].name; c++)
            if(strcmp(tokname, resrv[c].name) == 0)
                return resrv[c].value;
        error("invalid escape, or illegal reserved word: %s", tokname);
    }

    /* look ahead to distinguish IDENTIFIER from IDENTCOLON */
    c = Bgetrune(finput);
    while(c == ' ' || c == '\t'|| c == '\n' || c == '\f' || c == '/') {
        if(c == '\n')
            peekline++;
        /* look for comments */
        if(c == '/')
            peekline += skipcom();
        c = Bgetrune(finput);
    }
    if(c == ':')
        return IDENTCOLON;
    Bungetrune(finput);
    return IDENTIFIER;
}
/*e: function gettok */

/*s: function fdtype */
/*
 * determine the type of a symbol
 */
int
fdtype(int t)
{
    int v;

    if(t >= NTBASE)
        v = nontrst[t-NTBASE].value;
    else
        v = TYPE(toklev[t]);
    if(v <= 0)
        error("must specify type for %s", (t>=NTBASE)?
            nontrst[t-NTBASE].name: tokset[t].name);
    return v;
}
/*e: function fdtype */

/*s: function chfind */
int
chfind(int t, char *s)
{
    int i;

    if(s[0] == ' ')
        t = 0;
    TLOOP(i)
        if(!strcmp(s, tokset[i].name))
            return i;
    NTLOOP(i)
        if(!strcmp(s, nontrst[i].name))
            return NTBASE+i;

    /* cannot find name */
    if(t > 1)
        error("%s should have been defined earlier", s);
    return defin(t, s);
}
/*e: function chfind */

/*s: function cpyunion */
/*
 * copy the union declaration to the output, and the define file if present
 */
void
cpyunion(void)
{
    long c;
    int level;

    Bprint(ftable, "\n#line\t%d\t\"%s\"\n", lineno, infile);
    Bprint(ftable, "typedef union ");
    if(fdefine != 0)
        Bprint(fdefine, "\ntypedef union ");

    level = 0;
    for(;;) {
        if((c=Bgetrune(finput)) == Beof)
            error("EOF encountered while processing %%union");
        Bputrune(ftable, c);
        if(fdefine != 0)
            Bputrune(fdefine, c);
        switch(c) {
        case '\n':
            lineno++;
            break;
        case '{':
            level++;
            break;
        case '}':
            level--;

            /* we are finished copying */
            if(level == 0) {
                Bprint(ftable, " YYSTYPE;\n");
                if(fdefine != 0)
                    Bprint(fdefine, "\tYYSTYPE;\nextern\tYYSTYPE\tyylval;\n");
                return;
            }
        }
    }
}
/*e: function cpyunion */

/*s: function cpycode */
/*
 * copies code between \{ and \}
 */
void
cpycode(void)
{

    long c;

    c = Bgetrune(finput);
    if(c == '\n') {
        c = Bgetrune(finput);
        lineno++;
    }
    Bprint(ftable, "\n#line\t%d\t\"%s\"\n", lineno, infile);
    while(c != Beof) {
        if(c == '\\') {
            if((c=Bgetrune(finput)) == '}')
                return;
            Bputc(ftable, '\\');
        }
        if(c == '%') {
            if((c=Bgetrune(finput)) == '}')
                return;
            Bputc(ftable, '%');
        }
        Bputrune(ftable, c);
        if(c == '\n')
            lineno++;
        c = Bgetrune(finput);
    }
    error("eof before %%}");
}
/*e: function cpycode */

/*s: function skipcom */
/*
 * skip over comments
 * skipcom is called after reading a '/'
 */
int
skipcom(void)
{
    long c;
    int i;

    /* i is the number of lines skipped */
    i = 0;
    c = Bgetrune(finput);
    if(c == '/'){			/* C++ //: skip to end of line */
        while((c = Bgetrune(finput)) != Beof)
            if(c == '\n')
                return 1;
    }else if(c == '*'){		/* normal C comment */
        while((c = Bgetrune(finput)) != Beof) {
            while(c == '*')
                if((c = Bgetrune(finput)) == '/')
                    return i;
            if(c == '\n')
                i++;
        }
    }else
        error("illegal comment");

    error("EOF inside comment");
    return 0;
}
/*e: function skipcom */

/*s: function cpyact */
/*
 * copy C action to the next ; or closing }
 */
void
cpyact(int offset)
{
    long c;
    int brac, match, j, s, fnd, tok;

    Bprint(faction, "\n#line\t%d\t\"%s\"\n", lineno, infile);
    brac = 0;

loop:
    c = Bgetrune(finput);
swt:
    switch(c) {
    case ';':
        if(brac == 0) {
            Bputrune(faction, c);
            return;
        }
        goto lcopy;

    case '{':
        brac++;
        goto lcopy;

    case '$':
        s = 1;
        tok = -1;
        c = Bgetrune(finput);

        /* type description */
        if(c == '<') {
            Bungetrune(finput);
            if(gettok() != TYPENAME)
                error("bad syntax on $<ident> clause");
            tok = numbval;
            c = Bgetrune(finput);
        }
        if(c == '$') {
            Bprint(faction, "yyval");

            /* put out the proper tag... */
            if(ntypes) {
                if(tok < 0)
                    tok = fdtype(*prdptr[nprod]);
                Bprint(faction, ".%s", typeset[tok]);
            }
            goto loop;
        }
        if(c == '-') {
            s = -s;
            c = Bgetrune(finput);
        }
        if(isdigit(c)) {
            j = 0;
            while(isdigit(c)) {
                j = j*10 + (c-'0');
                c = Bgetrune(finput);
            }
            Bungetrune(finput);
            j = j*s - offset;
            if(j > 0)
                error("Illegal use of $%d", j+offset);

        dollar:
            Bprint(faction, "yypt[-%d].yyv", -j);

            /* put out the proper tag */
            if(ntypes) {
                if(j+offset <= 0 && tok < 0)
                    error("must specify type of $%d", j+offset);
                if(tok < 0)
                    tok = fdtype(prdptr[nprod][j+offset]);
                Bprint(faction, ".%s", typeset[tok]);
            }
            goto loop;
        }
        if(isupper(c) || islower(c) || c == '_' || c == '.') {
            int tok; /* tok used oustide for type info */

            /* look for $name */
            Bungetrune(finput);
            if(gettok() != IDENTIFIER)
                error("$ must be followed by an identifier");
            tok = chfind(2, tokname);
            if((c = Bgetrune(finput)) != '#') {
                Bungetrune(finput);
                fnd = -1;
            } else
                if(gettok() != NUMBER) {
                    error("# must be followed by number");
                    fnd = -1;
                } else
                    fnd = numbval;
            for(j=1; j<=offset; ++j)
                if(tok == prdptr[nprod][j]) {
                    if(--fnd <= 0) {
                        j -= offset;
                        goto dollar;
                    }
                }
            error("$name or $name#number not found");
        }
        Bputc(faction, '$');
        if(s < 0 )
            Bputc(faction, '-');
        goto swt;

    case '}':
        brac--;
        if(brac)
            goto lcopy;
        Bputrune(faction, c);
        return;

    case '/':
        /* look for comments */
        Bputrune(faction, c);
        c = Bgetrune(finput);
        if(c != '*')
            goto swt;

        /* it really is a comment; copy it */
        Bputrune(faction, c);
        c = Bgetrune(finput);
        while(c >= 0) {
            while(c == '*') {
                Bputrune(faction, c);
                if((c=Bgetrune(finput)) == '/')
                    goto lcopy;
            }
            Bputrune(faction, c);
            if(c == '\n')
                lineno++;
            c = Bgetrune(finput);
        }
        error("EOF inside comment");

    case '\'':
        /* character constant */
        match = '\'';
        goto string;

    case '"':
        /* character string */
        match = '"';

    string:
        Bputrune(faction, c);
        while(c = Bgetrune(finput)) {
            if(c == '\\') {
                Bputrune(faction, c);
                c = Bgetrune(finput);
                if(c == '\n')
                    lineno++;
            } else
                if(c == match)
                    goto lcopy;
                if(c == '\n')
                    error("newline in string or char. const.");
            Bputrune(faction, c);
        }
        error("EOF in string or character constant");

    case Beof:
        error("action does not terminate");

    case '\n':
        lineno++;
        goto lcopy;
    }

lcopy:
    Bputrune(faction, c);
    goto loop;
}
/*e: function cpyact */

/*s: function openup */
void
openup(char *stem, int dflag, int vflag, int ytab, char *ytabc)
{
    char buf[256];

    if(vflag) {
        snprint(buf, sizeof buf, "%s.%s", stem, FILEU);
        foutput = Bopen(buf, OWRITE);
        if(foutput == 0)
            error("cannot open %s", buf);
    }
    if(yydebug) {
        snprint(buf, sizeof buf, "%s.%s", stem, FILEDEBUG);
        if((fdebug = Bopen(buf, OWRITE)) == 0)
            error("can't open %s", buf);
    }
    if(dflag) {
        snprint(buf, sizeof buf, "%s.%s", stem, FILED);
        fdefine = Bopen(buf, OWRITE);
        if(fdefine == 0)
            error("can't create %s", buf);
    }
    if(ytab == 0)
        snprint(buf, sizeof buf, "%s.%s", stem, OFILE);
    else
        strecpy(buf, buf+sizeof buf, ytabc);
    ftable = Bopen(buf, OWRITE);
    if(ftable == 0)
        error("cannot open table file %s", buf);
}
/*e: function openup */

/*s: function output */
/*
 * print the output for the states
 */
void
output(void)
{
    int i, k, c;
    Wset *u, *v;

    Bprint(ftable, "short	yyexca[] =\n{");
    if(fdebug)
        Bprint(fdebug, "char*	yystates[] =\n{\n");

    /* output the stuff for state i */
    SLOOP(i) {
        nolook = tystate[i]!=MUSTLOOKAHEAD;
        closure(i);

        /* output actions */
        nolook = 1;
        aryfil(temp1, ntokens+nnonter+1, 0);
        WSLOOP(wsets, u) {
            c = *(u->pitem);
            if(c > 1 && c < NTBASE && temp1[c] == 0) {
                WSLOOP(u, v)
                    if(c == *(v->pitem))
                        putitem(v->pitem+1, (Lkset*)0);
                temp1[c] = state(c);
            } else
                if(c > NTBASE && temp1[(c -= NTBASE) + ntokens] == 0)
                    temp1[c+ntokens] = amem[indgo[i]+c];
        }
        if(i == 1)
            temp1[1] = ACCEPTCODE;

        /* now, we have the shifts; look at the reductions */
        lastred = 0;
        WSLOOP(wsets, u) {
            c = *u->pitem;

            /* reduction */
            if(c <= 0) {
                lastred = -c;
                TLOOP(k)
                    if(BIT(u->ws.lset, k)) {
                        if(temp1[k] == 0)
                            temp1[k] = c;
                        else
                        if(temp1[k] < 0) { /* reduce/reduce conflict */
                            if(foutput)
                                Bprint(foutput,
                                    "\n%d: reduce/reduce conflict"
                                    " (red'ns %d and %d ) on %s",
                                    i, -temp1[k], lastred,
                                    symnam(k));
                            if(-temp1[k] > lastred)
                                temp1[k] = -lastred;
                            zzrrconf++;
                        } else
                            /* potential shift/reduce conflict */
                            precftn( lastred, k, i );
                    }
                }
        }
        wract(i);
    }

    if(fdebug)
        Bprint(fdebug, "};\n");
    Bprint(ftable, "};\n");
    Bprint(ftable, "#define	YYNPROD	%d\n", nprod);
    Bprint(ftable, "#define	YYPRIVATE %d\n", PRIVATE);
    if(yydebug)
        Bprint(ftable, "#define	yydebug	%s\n", yydebug);
}
/*e: function output */

/*s: function apack */
/*
 * pack state i from temp1 into amem
 */
int
apack(int *p, int n)
{
    int *pp, *qq, *rr, off, *q, *r;

    /* we don't need to worry about checking because
     * we will only look at entries known to be there...
     * eliminate leading and trailing 0's
     */

    q = p+n;
    for(pp = p, off = 0; *pp == 0 && pp <= q; ++pp, --off)
        ;
  /* no actions */
    if(pp > q)
        return 0;
    p = pp;

    /* now, find a place for the elements from p to q, inclusive */
    r = &amem[ACTSIZE-1];
    for(rr = amem; rr <= r; rr++, off++) {
        for(qq = rr, pp = p; pp <= q; pp++, qq++)
            if(*pp != 0)
                if(*pp != *qq && *qq != 0)
                    goto nextk;

        /* we have found an acceptable k */
        if(pkdebug && foutput != 0)
            Bprint(foutput, "off = %d, k = %d\n", off, (int)(rr-amem));
        for(qq = rr, pp = p; pp <= q; pp++, qq++)
            if(*pp) {
                if(qq > r)
                    error("action table overflow");
                if(qq > memp)
                    memp = qq;
                *qq = *pp;
            }
        if(pkdebug && foutput != 0)
            for(pp = amem; pp <= memp; pp += 10) {
                Bprint(foutput, "\t");
                for(qq = pp; qq <= pp+9; qq++)
                    Bprint(foutput, "%d ", *qq);
                Bprint(foutput, "\n");
            }
        return(off);
    nextk:;
    }
    error("no space in action table");
    return 0;
}
/*e: function apack */

/*s: function go2out */
/*
 * output the gotos for the nontermninals
 */
void
go2out(void)
{
    int i, j, k, best, count, cbest, times;

    /* mark begining of gotos */
    Bprint(ftemp, "$\n");
    for(i = 1; i <= nnonter; i++) {
        go2gen(i);

        /* find the best one to make default */
        best = -1;
        times = 0;

        /* is j the most frequent */
        for(j = 0; j <= nstate; j++) {
            if(tystate[j] == 0)
                continue;
            if(tystate[j] == best)
                continue;

            /* is tystate[j] the most frequent */
            count = 0;
            cbest = tystate[j];
            for(k = j; k <= nstate; k++)
                if(tystate[k] == cbest)
                    count++;
            if(count > times) {
                best = cbest;
                times = count;
            }
        }

        /* best is now the default entry */
        zzgobest += times-1;
        for(j = 0; j <= nstate; j++)
            if(tystate[j] != 0 && tystate[j] != best) {
                Bprint(ftemp, "%d,%d,", j, tystate[j]);
                zzgoent++;
            }

        /* now, the default */
        if(best == -1)
            best = 0;
        zzgoent++;
        Bprint(ftemp, "%d\n", best);
    }
}
/*e: function go2out */

/*s: function go2gen */
/*
 * output the gotos for nonterminal c
 */
void
go2gen(int c)
{
    int i, work, cc;
    Item *p, *q;


    /* first, find nonterminals with gotos on c */
    aryfil(temp1, nnonter+1, 0);
    temp1[c] = 1;
    work = 1;
    while(work) {
        work = 0;
        PLOOP(0, i)

            /* cc is a nonterminal */
            if((cc=prdptr[i][1]-NTBASE) >= 0)
                /* cc has a goto on c */
                if(temp1[cc] != 0) {

                    /* thus, the left side of production i does too */
                    cc = *prdptr[i]-NTBASE;
                    if(temp1[cc] == 0) {
                          work = 1;
                          temp1[cc] = 1;
                    }
                }
    }

    /* now, we have temp1[c] = 1 if a goto on c in closure of cc */
    if(g2debug && foutput != 0) {
        Bprint(foutput, "%s: gotos on ", nontrst[c].name);
        NTLOOP(i)
            if(temp1[i])
                Bprint(foutput, "%s ", nontrst[i].name);
        Bprint(foutput, "\n");
    }

    /* now, go through and put gotos into tystate */
    aryfil(tystate, nstate, 0);
    SLOOP(i)
        ITMLOOP(i, p, q)
            if((cc = *p->pitem) >= NTBASE)
                /* goto on c is possible */
                if(temp1[cc-NTBASE]) {
                    tystate[i] = amem[indgo[i]+c];
                    break;
                }
}
/*e: function go2gen */

/*s: function precftn */
/*
 * decide a shift/reduce conflict by precedence.
 * r is a rule number, t a token number
 * the conflict is in state s
 * temp1[t] is changed to reflect the action
 */
void
precftn(int r, int t, int s)
{
    int lp, lt, action;

    lp = levprd[r];
    lt = toklev[t];
    if(PLEVEL(lt) == 0 || PLEVEL(lp) == 0) {

        /* conflict */
        if(foutput != 0)
            Bprint(foutput,
                "\n%d: shift/reduce conflict (shift %d(%d), red'n %d(%d)) on %s",
                s, temp1[t], PLEVEL(lt), r, PLEVEL(lp), symnam(t));
        zzsrconf++;
        return;
    }
    if(PLEVEL(lt) == PLEVEL(lp))
        action = ASSOC(lt);
    else
        if(PLEVEL(lt) > PLEVEL(lp))
            action = RASC;  /* shift */
        else
            action = LASC;  /* reduce */
    switch(action) {
    case BASC:  /* error action */
        temp1[t] = ERRCODE;
        break;
    case LASC:  /* reduce */
        temp1[t] = -r;
        break;
    }
}
/*e: function precftn */

/*s: function wract */
/*
 * output state i
 * temp1 has the actions, lastred the default
 */
void
wract(int i)
{
    int p, p0, p1, ntimes, tred, count, j, flag;

    /* find the best choice for lastred */
    lastred = 0;
    ntimes = 0;
    TLOOP(j) {
        if(temp1[j] >= 0)
            continue;
        if(temp1[j]+lastred == 0)
            continue;
        /* count the number of appearances of temp1[j] */
        count = 0;
        tred = -temp1[j];
        levprd[tred] |= REDFLAG;
        TLOOP(p)
            if(temp1[p]+tred == 0)
                count++;
        if(count > ntimes) {
            lastred = tred;
            ntimes = count;
        }
    }

    /*
     * for error recovery, arrange that, if there is a shift on the
     * error recovery token, `error', that the default be the error action
     */
    if(temp1[2] > 0)
        lastred = 0;

    /* clear out entries in temp1 which equal lastred */
    TLOOP(p)
        if(temp1[p]+lastred == 0)
            temp1[p] = 0;

    wrstate(i);
    defact[i] = lastred;
    flag = 0;
    TLOOP(p0)
        if((p1=temp1[p0]) != 0) {
            if(p1 < 0) {
                p1 = -p1;
                goto exc;
            }
            if(p1 == ACCEPTCODE) {
                p1 = -1;
                goto exc;
            }
            if(p1 == ERRCODE) {
                p1 = 0;
            exc:
                if(flag++ == 0)
                    Bprint(ftable, "-1, %d,\n", i);
                Bprint(ftable, "\t%d, %d,\n", p0, p1);
                zzexcp++;
                continue;
            }
            Bprint(ftemp, "%d,%d,", p0, p1);
            zzacent++;
        }
    if(flag) {
        defact[i] = -2;
        Bprint(ftable, "\t-2, %d,\n", lastred);
    }
    Bprint(ftemp, "\n");
}
/*e: function wract */

/*s: function wrstate */
/*
 * writes state i
 */
void
wrstate(int i)
{
    int j0, j1;
    Item *pp, *qq;
    Wset *u;

    if(fdebug) {
        if(lastred) {
            Bprint(fdebug, "	0, /*%d*/\n", i);
        } else {
            Bprint(fdebug, "	\"");
            ITMLOOP(i, pp, qq)
                Bprint(fdebug, "%s\\n", writem(pp->pitem));
            if(tystate[i] == MUSTLOOKAHEAD)
                WSLOOP(wsets + (pstate[i+1] - pstate[i]), u)
                    if(*u->pitem < 0)
                        Bprint(fdebug, "%s\\n", writem(u->pitem));
            Bprint(fdebug, "\", /*%d*/\n", i);
        }
    }
    if(foutput == 0)
        return;
    Bprint(foutput, "\nstate %d\n", i);
    ITMLOOP(i, pp, qq)
        Bprint(foutput, "\t%s\n", writem(pp->pitem));
    if(tystate[i] == MUSTLOOKAHEAD)
        /* print out empty productions in closure */
        WSLOOP(wsets+(pstate[i+1]-pstate[i]), u)
            if(*u->pitem < 0)
                Bprint(foutput, "\t%s\n", writem(u->pitem));

    /* check for state equal to another */
    TLOOP(j0)
        if((j1=temp1[j0]) != 0) {
            Bprint(foutput, "\n\t%s  ", symnam(j0));
            /* shift, error, or accept */
            if(j1 > 0) {
                if(j1 == ACCEPTCODE)
                    Bprint(foutput,  "accept");
                else
                    if(j1 == ERRCODE)
                        Bprint(foutput, "error");
                    else
                        Bprint(foutput, "shift %d", j1);
            } else
                Bprint(foutput, "reduce %d (src line %d)", -j1, rlines[-j1]);
        }

    /* output the final production */
    if(lastred)
        Bprint(foutput, "\n\t.  reduce %d (src line %d)\n\n",
            lastred, rlines[lastred]);
    else
        Bprint(foutput, "\n\t.  error\n\n");

    /* now, output nonterminal actions */
    j1 = ntokens;
    for(j0 = 1; j0 <= nnonter; j0++) {
        j1++;
        if(temp1[j1])
            Bprint(foutput, "\t%s  goto %d\n", symnam(j0+NTBASE), temp1[j1]);
    }
}
/*e: function wrstate */

/*s: function warray */
void
warray(char *s, int *v, int n)
{
    int i;

    Bprint(ftable, "short	%s[] =\n{", s);
    for(i=0;;) {
        if(i%10 == 0)
            Bprint(ftable, "\n");
        Bprint(ftable, "%4d", v[i]);
        i++;
        if(i >= n) {
            Bprint(ftable, "\n};\n");
            break;
        }
        Bprint(ftable, ",");
    }
}
/*e: function warray */

/*s: function hideprod */
/*
 * in order to free up the mem and amem arrays for the optimizer,
 * and still be able to output yyr1, etc., after the sizes of
 * the action array is known, we hide the nonterminals
 * derived by productions in levprd.
 */

void
hideprod(void)
{
    int i, j;

    j = 0;
    levprd[0] = 0;
    PLOOP(1, i) {
        if(!(levprd[i] & REDFLAG)) {
            j++;
            if(foutput != 0)
                Bprint(foutput, "Rule not reduced:   %s\n", writem(prdptr[i]));
        }
        levprd[i] = *prdptr[i] - NTBASE;
    }
    if(j)
        print("%d rules never reduced\n", j);
}
/*e: function hideprod */

/*s: function callopt */
void
callopt(void)
{
    int i, *p, j, k, *q;

    /* read the arrays from tempfile and set parameters */
    finput = Bopen(tempname, OREAD);
    if(finput == 0)
        error("optimizer cannot open tempfile");

    pgo[0] = 0;
    temp1[0] = 0;
    nstate = 0;
    nnonter = 0;
    for(;;) {
        switch(gtnm()) {
        case '\n':
            nstate++;
            pmem--;
            temp1[nstate] = pmem - mem0;
        case ',':
            continue;
        case '$':
            break;
        default:
            error("bad tempfile");
        }
        break;
    }

    pmem--;
    temp1[nstate] = yypgo[0] = pmem - mem0;
    for(;;) {
        switch(gtnm()) {
        case '\n':
            nnonter++;
            yypgo[nnonter] = pmem-mem0;
        case ',':
            continue;
        case -1:
            break;
        default:
            error("bad tempfile");
        }
        break;
    }
    pmem--;
    yypgo[nnonter--] = pmem - mem0;
    for(i = 0; i < nstate; i++) {
        k = 32000;
        j = 0;
        q = mem0 + temp1[i+1];
        for(p = mem0 + temp1[i]; p < q ; p += 2) {
            if(*p > j)
                j = *p;
            if(*p < k)
                k = *p;
        }
        /* nontrivial situation */
        if(k <= j) {
            /* j is now the range */
/*			j -= k;			/* call scj */
            if(k > maxoff)
                maxoff = k;
        }
        tystate[i] = (temp1[i+1]-temp1[i]) + 2*j;
        if(j > maxspr)
            maxspr = j;
    }

    /* initialize ggreed table */
    for(i = 1; i <= nnonter; i++) {
        ggreed[i] = 1;
        j = 0;

        /* minimum entry index is always 0 */
        q = mem0 + yypgo[i+1] - 1;
        for(p = mem0+yypgo[i]; p < q ; p += 2) {
            ggreed[i] += 2;
            if(*p > j)
                j = *p;
        }
        ggreed[i] = ggreed[i] + 2*j;
        if(j > maxoff)
            maxoff = j;
    }

    /* now, prepare to put the shift actions into the amem array */
    for(i = 0; i < ACTSIZE; i++)
        amem[i] = 0;
    maxa = amem;
    for(i = 0; i < nstate; i++) {
        if(tystate[i] == 0 && adb > 1)
            Bprint(ftable, "State %d: null\n", i);
        indgo[i] = YYFLAG1;
    }
    while((i = nxti()) != NOMORE)
        if(i >= 0)
            stin(i);
        else
            gin(-i);

    /* print amem array */
    if(adb > 2 )
        for(p = amem; p <= maxa; p += 10) {
            Bprint(ftable, "%4d  ", (int)(p-amem));
            for(i = 0; i < 10; ++i)
                Bprint(ftable, "%4d  ", p[i]);
            Bprint(ftable, "\n");
        }

    /* write out the output appropriate to the language */
    aoutput();
    osummary();
    ZAPFILE(tempname);
}
/*e: function callopt */

/*s: function gin */
void
gin(int i)
{
    int *p, *r, *s, *q1, *q2;

    /* enter gotos on nonterminal i into array amem */
    ggreed[i] = 0;

    q2 = mem0+ yypgo[i+1] - 1;
    q1 = mem0 + yypgo[i];

    /* now, find amem place for it */
    for(p = amem; p < &amem[ACTSIZE]; p++) {
        if(*p)
            continue;
        for(r = q1; r < q2; r += 2) {
            s = p + *r + 1;
            if(*s)
                goto nextgp;
            if(s > maxa)
                if((maxa = s) > &amem[ACTSIZE])
                    error("a array overflow");
        }
        /* we have found amem spot */
        *p = *q2;
        if(p > maxa)
            if((maxa = p) > &amem[ACTSIZE])
                error("a array overflow");
        for(r = q1; r < q2; r += 2) {
            s = p + *r + 1;
            *s = r[1];
        }
        pgo[i] = p-amem;
        if(adb > 1)
            Bprint(ftable, "Nonterminal %d, entry at %d\n", i, pgo[i]);
        return;

    nextgp:;
    }
    error("cannot place goto %d\n", i);
}
/*e: function gin */

/*s: function stin */
void
stin(int i)
{
    int *r, *s, n, flag, j, *q1, *q2;

    tystate[i] = 0;

    /* enter state i into the amem array */
    q2 = mem0+temp1[i+1];
    q1 = mem0+temp1[i];
    /* find an acceptable place */
    for(n = -maxoff; n < ACTSIZE; n++) {
        flag = 0;
        for(r = q1; r < q2; r += 2) {
            if((s = *r + n + amem) < amem)
                goto nextn;
            if(*s == 0)
                flag++;
            else
                if(*s != r[1])
                    goto nextn;
        }

        /* check that the position equals another only if the states are identical */
        for(j=0; j<nstate; j++) {
            if(indgo[j] == n) {

                /* we have some disagreement */
                if(flag)
                    goto nextn;
                if(temp1[j+1]+temp1[i] == temp1[j]+temp1[i+1]) {

                    /* states are equal */
                    indgo[i] = n;
                    if(adb > 1)
                        Bprint(ftable,
                        "State %d: entry at %d equals state %d\n",
                        i, n, j);
                    return;
                }

                /* we have some disagreement */
                goto nextn;
            }
        }

        for(r = q1; r < q2; r += 2) {
            if((s = *r+n+amem) >= &amem[ACTSIZE])
                error("out of space in optimizer a array");
            if(s > maxa)
                maxa = s;
            if(*s != 0 && *s != r[1])
                error("clobber of a array, pos'n %d, by %d", s-amem, r[1]);
            *s = r[1];
        }
        indgo[i] = n;
        if(adb > 1)
            Bprint(ftable, "State %d: entry at %d\n", i, indgo[i]);
        return;
    nextn:;
    }
    error("Error; failure to place state %d\n", i);
}
/*e: function stin */

/*s: function nxti */
/*
 * finds the next i
 */
int
nxti(void)
{
    int i, max, maxi;

    max = 0;
    maxi = 0;
    for(i = 1; i <= nnonter; i++)
        if(ggreed[i] >= max) {
            max = ggreed[i];
            maxi = -i;
        }
    for(i = 0; i < nstate; ++i)
        if(tystate[i] >= max) {
            max = tystate[i];
            maxi = i;
        }
    if(nxdb)
        Bprint(ftable, "nxti = %d, max = %d\n", maxi, max);
    if(max == 0)
        return NOMORE;
    return maxi;
}
/*e: function nxti */

/*s: function osummary */
/*
 * write summary
 */
void
osummary(void)
{

    int i, *p;

    if(foutput == 0)
        return;
    i = 0;
    for(p = maxa; p >= amem; p--)
        if(*p == 0)
            i++;

    Bprint(foutput, "Optimizer space used: input %d/%d, output %d/%d\n",
        (int)(pmem-mem0+1), MEMSIZE, (int)(maxa-amem+1), ACTSIZE);
    Bprint(foutput, "%d table entries, %d zero\n", (int)(maxa-amem+1), i);
    Bprint(foutput, "maximum spread: %d, maximum offset: %d\n", maxspr, maxoff);
}
/*e: function osummary */

/*s: function aoutput */
/*
 * this version is for C
 * write out the optimized parser
 */
void
aoutput(void)
{
    Bprint(ftable, "#define\tYYLAST\t%d\n", (int)(maxa-amem+1));
    arout("yyact", amem, (maxa-amem)+1);
    arout("yypact", indgo, nstate);
    arout("yypgo", pgo, nnonter+1);
}
/*e: function aoutput */

/*s: function arout */
void
arout(char *s, int *v, int n)
{
    int i;

    Bprint(ftable, "short	%s[] =\n{", s);
    for(i = 0; i < n;) {
        if(i%10 == 0)
            Bprint(ftable, "\n");
        Bprint(ftable, "%4d", v[i]);
        i++;
        if(i == n)
            Bprint(ftable, "\n};\n");
        else
            Bprint(ftable, ",");
    }
}
/*e: function arout */

/*s: function gtnm */
/*
 * read and convert an integer from the standard input
 * return the terminating character
 * blanks, tabs, and newlines are ignored
 */
int
gtnm(void)
{
    int sign, val, c;

    sign = 0;
    val = 0;
    while((c=Bgetrune(finput)) != Beof) {
        if(isdigit(c)) {
            val = val*10 + c-'0';
            continue;
        }
        if(c == '-') {
            sign = 1;
            continue;
        }
        break;
    }
    if(sign)
        val = -val;
    *pmem++ = val;
    if(pmem >= &mem0[MEMSIZE])
        error("out of space");
    return c;
}
/*e: function gtnm */
/*e: generators/yacc/yacc.c */
