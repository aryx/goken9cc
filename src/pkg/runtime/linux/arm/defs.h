// godefs -carm-gcc -f -I/usr/local/google/src/linux-2.6.28/arch/arm/include -f -I/usr/local/google/src/linux-2.6.28/include -f-D__KERNEL__ -f-D__ARCH_SI_UID_T=int defs_arm.c

// MACHINE GENERATED - DO NOT EDIT.

// Constants
enum {
	PROT_NONE = 0,
	PROT_READ = 0x1,
	PROT_WRITE = 0x2,
	PROT_EXEC = 0x4,
	MAP_ANON = 0x20,
	MAP_PRIVATE = 0x2,
	MAP_FIXED = 0x10,
	SA_RESTART = 0x10000000,
	SA_ONSTACK = 0x8000000,
	SA_RESTORER = 0x4000000,
	SA_SIGINFO = 0x4,
	SIGHUP = 0x1,
	SIGINT = 0x2,
	SIGQUIT = 0x3,
	SIGILL = 0x4,
	SIGTRAP = 0x5,
	SIGABRT = 0x6,
	SIGBUS = 0x7,
	SIGFPE = 0x8,
	SIGKILL = 0x9,
	SIGUSR1 = 0xa,
	SIGSEGV = 0xb,
	SIGUSR2 = 0xc,
	SIGPIPE = 0xd,
	SIGALRM = 0xe,
	SIGSTKFLT = 0x10,
	SIGCHLD = 0x11,
	SIGCONT = 0x12,
	SIGSTOP = 0x13,
	SIGTSTP = 0x14,
	SIGTTIN = 0x15,
	SIGTTOU = 0x16,
	SIGURG = 0x17,
	SIGXCPU = 0x18,
	SIGXFSZ = 0x19,
	SIGVTALRM = 0x1a,
	SIGPROF = 0x1b,
	SIGWINCH = 0x1c,
	SIGIO = 0x1d,
	SIGPWR = 0x1e,
	SIGSYS = 0x1f,
	FPE_INTDIV = 0x30001,
	FPE_INTOVF = 0x30002,
	FPE_FLTDIV = 0x30003,
	FPE_FLTOVF = 0x30004,
	FPE_FLTUND = 0x30005,
	FPE_FLTRES = 0x30006,
	FPE_FLTINV = 0x30007,
	FPE_FLTSUB = 0x30008,
	BUS_ADRALN = 0x30001,
	BUS_ADRERR = 0x30002,
	BUS_OBJERR = 0x30003,
	SEGV_MAPERR = 0x30001,
	SEGV_ACCERR = 0x30002,
};

// Types
#pragma pack on

typedef struct Sigset Sigset;
struct Sigset {
	uint32 sig[2];
};

typedef struct Sigaction Sigaction;
struct Sigaction {
	void *sa_handler;
	uint32 sa_flags;
	void *sa_restorer;
	Sigset sa_mask;
};

typedef struct Timespec Timespec;
struct Timespec {
	int32 tv_sec;
	int32 tv_nsec;
};

typedef struct Sigaltstack Sigaltstack;
struct Sigaltstack {
	void *ss_sp;
	int32 ss_flags;
	uint32 ss_size;
};

typedef struct Sigcontext Sigcontext;
struct Sigcontext {
	uint32 trap_no;
	uint32 error_code;
	uint32 oldmask;
	uint32 arm_r0;
	uint32 arm_r1;
	uint32 arm_r2;
	uint32 arm_r3;
	uint32 arm_r4;
	uint32 arm_r5;
	uint32 arm_r6;
	uint32 arm_r7;
	uint32 arm_r8;
	uint32 arm_r9;
	uint32 arm_r10;
	uint32 arm_fp;
	uint32 arm_ip;
	uint32 arm_sp;
	uint32 arm_lr;
	uint32 arm_pc;
	uint32 arm_cpsr;
	uint32 fault_address;
};

typedef struct Ucontext Ucontext;
struct Ucontext {
	uint32 uc_flags;
	Ucontext *uc_link;
	Sigaltstack uc_stack;
	Sigcontext uc_mcontext;
	Sigset uc_sigmask;
	int32 __unused[30];
	uint32 uc_regspace[128];
};

typedef struct Siginfo Siginfo;
struct Siginfo {
	int32 si_signo;
	int32 si_errno;
	int32 si_code;
	uint8 _sifields[4];
};
#pragma pack off
