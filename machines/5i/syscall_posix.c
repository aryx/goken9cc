#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

#include "arm.h"

//#define	ODIRLEN	116	/* compatibility; used in _stat etc. */
#define	OERRLEN	64	/* compatibility; used in _stat etc. */

char 	errbuf[ERRMAX];
ulong	nofunc;

//#include "../../lib_core/libc/9syscall/sys.h"
#include "sys.h"

char*	sysctab[] =
{
    [NOP]		"Nop",
    [BIND]		"Bind",
    [CHDIR]		"Chdir",
    [CLOSE]		"Close",
    [DUP]		"Dup",
    [ALARM]		"Alarm",
    [EXEC]		"Exec",
    [EXITS]		"Exits",
    [FAUTH]		"Fauth",
    [SEGBRK]	"Segbrk",
    [MOUNT]		"Mount",
    [OPEN]		"Open",
    [SLEEP]		"Sleep",
    [RFORK]		"Rfork",
    [PIPE]		"Pipe",
    [CREATE]	"Create",
    [FD2PATH]	"Fd2path",
    [BRK]		"Brk",
    [REMOVE]	"Remove",
    [NOTIFY]	"Notify",
    [NOTED]		"Noted",
    [SEGATTACH]		"Segattach",
    [SEGDETACH]		"Segdetach",
    [SEGFREE]		"Segfree",
    [SEGFLUSH]		"Segflush",
    [RENDEZVOUS]	"Rendezvous",
    [UNMOUNT]		"Unmount",
    [SEEK]		"Seek",
    [FVERSION]	"Fversion",
    [ERRSTR]	"Errstr",
    [STAT]		"Stat",
    [FSTAT]		"Fstat",
    [WSTAT]		"Wstat",
    [FWSTAT]	"Fwstat",
    [PREAD]		"Pread",
    [PWRITE]	"Pwrite",
    [AWAIT]		"Await",
};


void
sysnop(void)
{
    Bprint(bout, "nop system call %s\n", sysctab[reg.r[1]]);
    if(sysdbg)
        itrace("nop()");
}

void
syserrstr(void)
{
    ulong str;
    int n;

    str = getmem_w(reg.r[REGSP]+4);
    n = getmem_w(reg.r[REGSP]+8);
    if(sysdbg)
        itrace("errstr(0x%lux, 0x%lux)", str, n);

    if(n > strlen(errbuf)+1)
        n = strlen(errbuf)+1;
    memio(errbuf, str, n, MemWrite);
    strcpy(errbuf, "no error");
    reg.r[REGRET] = n;
    
}

void
sysbind(void)
{ 
    ulong pname, pold, flags;
    char name[1024], old[1024];
    int n;

    pname = getmem_w(reg.r[REGSP]+4);
    pold = getmem_w(reg.r[REGSP]+8);
    flags = getmem_w(reg.r[REGSP]+12);
    memio(name, pname, sizeof(name), MemReadstring);
    memio(old, pold, sizeof(old), MemReadstring);
    if(sysdbg)
        itrace("bind(0x%lux='%s', 0x%lux='%s', 0x%lux)", name, name, old, old, flags);

    n = bind(name, old, flags);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);

    reg.r[REGRET] = n;
}

void
sysfd2path(void)
{
    int n;
    uint fd;
    ulong str;
    char buf[1024];

    fd = getmem_w(reg.r[REGSP]+4);
    str = getmem_w(reg.r[REGSP]+8);
    n = getmem_w(reg.r[REGSP]+12);
    if(sysdbg)
        itrace("fd2path(0x%lux, 0x%lux, 0x%lux)", fd, str, n);
    reg.r[1] = -1;
    if(n > sizeof buf){
        strcpy(errbuf, "buffer too big");
        return;
    }
    //XXX: n = fd2path(fd, buf, sizeof buf);
    n = -1;
    if(n < 0)
        errstr(buf, sizeof buf);
    else
        memio(errbuf, str, n, MemWrite);
    reg.r[REGRET] = n;
    
}

void
syschdir(void)
{ 
    char file[1024];
    int n;
    ulong name;

    name = getmem_w(reg.r[REGSP]+4);
    memio(file, name, sizeof(file), MemReadstring);
    if(sysdbg)
        itrace("chdir(0x%lux='%s', 0x%lux)", name, file);
    
    n = chdir(file);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);

    reg.r[REGRET] = n;
}

void
sysclose(void)
{
    int n;
    ulong fd;

    fd = getmem_w(reg.r[REGSP]+4);
    if(sysdbg)
        itrace("close(%d)", fd);

    n = close(fd);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);
    reg.r[REGRET] = n;
}

void
sysdup(void)
{
    int oldfd, newfd;
    int n;

    oldfd = getmem_w(reg.r[REGSP]+4);
    newfd = getmem_w(reg.r[REGSP]+8);
    if(sysdbg)
        itrace("dup(%d, %d)", oldfd, newfd);

    n = dup(oldfd, newfd);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);
    reg.r[REGRET] = n;
}

void
sysexits(void)
{
    char buf[OERRLEN];
    ulong str;

    str = getmem_w(reg.r[REGSP]+4);
    if(sysdbg)
        itrace("exits(0x%lux)", str);

    // single step to give opportunity to inspect before exit
    count = 1;
    if(str != 0) {
        memio(buf, str, sizeof buf, MemRead);
        Bprint(bout, "exits(%s)\n", buf);
    }
    else
        Bprint(bout, "exits(0)\n");
}

void
sysopen(void)
{
    char file[1024];
    int n;
    ulong mode, name;

    name = getmem_w(reg.r[REGSP]+4);
    mode = getmem_w(reg.r[REGSP]+8);
    memio(file, name, sizeof(file), MemReadstring);
    
    n = open(file, mode);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);

    if(sysdbg)
        itrace("open(0x%lux='%s', 0x%lux) = %d", name, file, mode, n);

    reg.r[REGRET] = n;
};

void
sysread(vlong offset)
{
    int fd;
    ulong size, a;
    char *buf, *p;
    int n, cnt, c;

    fd = getmem_w(reg.r[REGSP]+4);
    a = getmem_w(reg.r[REGSP]+8);
    size = getmem_w(reg.r[REGSP]+12);

    buf = emalloc(size);
    if(fd == 0) {
        print("\nstdin>>");
        p = buf;
        n = 0;
        cnt = size;
        while(cnt) {
            c = Bgetc(bin);
            if(c <= 0)
                break;
            *p++ = c;
            n++;
            cnt--;
            if(c == '\n')
                break;
        }
    }
    else
        n = pread(fd, buf, size, offset);

    if(n < 0)
        errstr(errbuf, sizeof errbuf);
    else
        memio(buf, a, n, MemWrite);

    if(sysdbg)
        itrace("read(%d, 0x%lux, %d, 0x%llx) = %d", fd, a, size, offset, n);

    free(buf);
    reg.r[REGRET] = n;
}

void
syspread(void)
{
    sysread(getmem_v(reg.r[REGSP]+16));
}

void
sysseek(void)
{
    int fd;
    ulong mode;
    ulong retp;
    vlong v;

    retp = getmem_w(reg.r[REGSP]+4);
    fd = getmem_w(reg.r[REGSP]+8);
    v = getmem_v(reg.r[REGSP]+16);
    mode = getmem_w(reg.r[REGSP]+20);
    if(sysdbg)
        itrace("seek(%d, %lld, %d)", fd, v, mode);

    v = seek(fd, v, mode);
    if(v < 0)
        errstr(errbuf, sizeof errbuf);	

    putmem_v(retp, v);
}

void
syssleep(void)
{
    ulong len;
    int n;

    len = getmem_w(reg.r[REGSP]+4);
    if(sysdbg)
        itrace("sleep(%d)", len);

    //XXX: n = sleep(len);
    n = -1;
    if(n < 0)
        errstr(errbuf, sizeof errbuf);	

    reg.r[REGRET] = n;
}

void
sysstat(void)
{
    char nambuf[1024];
    byte buf[STATMAX];
    ulong edir, name;
    int n;

    name = getmem_w(reg.r[REGSP]+4);
    edir = getmem_w(reg.r[REGSP]+8);
    n = getmem_w(reg.r[REGSP]+12);
    memio(nambuf, name, sizeof(nambuf), MemReadstring);
    if(sysdbg)
        itrace("stat(0x%lux='%s', 0x%lux, 0x%lux)", name, nambuf, edir, n);
    if(n > sizeof buf)
        errstr(errbuf, sizeof errbuf);
    else{	
        n = stat(nambuf, buf, n);
        if(n < 0)
            errstr(errbuf, sizeof errbuf);
        else
            memio((char*)buf, edir, n, MemWrite);
    }
    reg.r[REGRET] = n;
}

void
sysfstat(void)
{
    byte buf[STATMAX];
    ulong edir;
    int n, fd;

    fd = getmem_w(reg.r[REGSP]+4);
    edir = getmem_w(reg.r[REGSP]+8);
    n = getmem_w(reg.r[REGSP]+12);
    if(sysdbg)
        itrace("fstat(%d, 0x%lux, 0x%lux)", fd, edir, n);

    reg.r[REGRET] = -1;
    if(n > sizeof buf){
        strcpy(errbuf, "stat buffer too big");
        return;
    }
    n = fstat(fd, buf, n);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);
    else
        memio((char*)buf, edir, n, MemWrite);
    reg.r[REGRET] = n;
}

void
syswrite(vlong offset)
{
    int fd;
    ulong size, a;
    char *buf;
    int n;

    fd = getmem_w(reg.r[REGSP]+4);
    a = getmem_w(reg.r[REGSP]+8);
    size = getmem_w(reg.r[REGSP]+12);

    Bflush(bout);
    buf = memio(0, a, size, MemRead);
    n = pwrite(fd, buf, size, offset);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);	

    if(sysdbg)
        itrace("write(%d, %lux, %d, 0x%llx) = %d", fd, a, size, offset, n);

    free(buf);

    reg.r[REGRET] = n;
}

void
syspwrite(void)
{
    syswrite(getmem_v(reg.r[REGSP]+16));
}

void
syspipe(void)
{
    int n, p[2];
    ulong fd;

    fd = getmem_w(reg.r[REGSP]+4);
    if(sysdbg)
        itrace("pipe(%lux)", fd);

    n = pipe(p);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);
    else {
        putmem_w(fd, p[0]);
        putmem_w(fd+4, p[1]);
    }
    reg.r[REGRET] = n;
}

void
syscreate(void)
{
    char file[1024];
    int n;
    ulong mode, name, perm;

    name = getmem_w(reg.r[REGSP]+4);
    mode = getmem_w(reg.r[REGSP]+8);
    perm = getmem_w(reg.r[REGSP]+12);
    memio(file, name, sizeof(file), MemReadstring);
    if(sysdbg)
        itrace("create(0x%lux='%s', 0x%lux, 0x%lux)", name, file, mode, perm);
    
    n = create(file, mode, perm);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);

    reg.r[REGRET] = n;
}

void
sysbrk(void)
{
    ulong addr, osize, nsize;
    Segment *s;

    addr = getmem_w(reg.r[REGSP]+4);
    if(sysdbg)
        itrace("brk(0x%lux)", addr);

    reg.r[REGRET] = -1;
    if(addr < memory.seg[Data].base+datasize) {
        strcpy(errbuf, "address below segment");
        return;
    }
    if(addr > memory.seg[Stack].base) {
        strcpy(errbuf, "segment too big");
        return;
    }
    s = &memory.seg[Bss];
    if(addr > s->end) {
        osize = ((s->end-s->base)/BY2PG)*sizeof(byte*);
        addr = ((addr)+(BY2PG-1))&~(BY2PG-1);
        s->end = addr;
        nsize = ((s->end-s->base)/BY2PG)*sizeof(byte*);
        s->table = erealloc(s->table, osize, nsize);
    }	

    reg.r[REGRET] = 0;	
}

void
sysremove(void)
{
    char nambuf[1024];
    ulong name;
    int n;

    name = getmem_w(reg.r[REGSP]+4);
    memio(nambuf, name, sizeof(nambuf), MemReadstring);
    if(sysdbg)
        itrace("remove(0x%lux='%s')", name, nambuf);

    n = remove(nambuf);
    if(n < 0)
        errstr(errbuf, sizeof errbuf);
    reg.r[REGRET] = n;
}

void
sysnotify(void)
{
    nofunc = getmem_w(reg.r[REGSP]+4);
    if(sysdbg)
        itrace("notify(0x%lux)\n", nofunc);

    reg.r[REGRET] = 0;
}




void
sysawait(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

void
sysrfork(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

void
syswstat(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}


void
sysfwstat(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}


void
sysnoted(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}


void
syssegattach(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

void
syssegdetach(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

void
syssegfree(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

void
syssegflush(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}
void
sysrendezvous(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}
void
sysunmount(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

void
syssegbrk(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}
void
sysmount(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}
void
sysalarm(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}
void
sysexec(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

void
sysfauth(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}
void
sysfversion(void)
{
    Bprint(bout, "No system call %s\n", sysctab[reg.r[REGARG]]);
    exits(0);
}

/*s: global [[systab]] */
void	(*systab[])(void) =
{
    [NOP]		sysnop,

    [RFORK]		sysrfork,
    [EXEC]		sysexec,
    [EXITS]		sysexits,
    [AWAIT]		sysawait,

    [BRK]		sysbrk,

    [OPEN]		sysopen,
    [CLOSE]		sysclose,
    [PREAD]		syspread,
    [PWRITE]	syspwrite,
    [SEEK]		sysseek,

    [CREATE]	syscreate,
    [REMOVE]	sysremove,
    [CHDIR]		syschdir,
    [FD2PATH]	sysfd2path,
    [STAT]		sysstat,
    [FSTAT]		sysfstat,
    [WSTAT]		syswstat,
    [FWSTAT]	sysfwstat,

    [BIND]		sysbind,
    [MOUNT]		sysmount,
    [UNMOUNT]	sysunmount,

    [SLEEP]		syssleep,
    [ALARM]		sysalarm,

    [PIPE]		syspipe,
    [NOTIFY]	sysnotify,
    [NOTED]		sysnoted,

    [SEGATTACH]	syssegattach,
    [SEGDETACH]	syssegdetach,
    [SEGFREE]	syssegfree,
    [SEGFLUSH]	syssegflush,
    [SEGBRK]	syssegbrk,

    [RENDEZVOUS]	sysrendezvous,

    [DUP]		sysdup,
    [FVERSION]	sysfversion,
    [FAUTH]		sysfauth,

    [ERRSTR]	syserrstr,
};

void
Ssyscall(instruction _unused)
{
    int call;
    USED(_unused);

    call = reg.r[REGARG];

    if(call < 0 || call >= nelem(systab) || systab[call] == nil) {
        Bprint(bout, "bad system call %d (%#ux)\n", call, call);
        dumpreg();
        Bflush(bout);
        return;
    }

    if(trace)
        itrace("SWI\t%s", sysctab[call]);

    // dispatch!
    (*systab[call])();

    Bflush(bout);
}
