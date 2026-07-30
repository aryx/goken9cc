// The only raw syscall entry point for darwin/amd64: loads up to 6 args
// into the registers XNU's SYSCALL handler expects, adds the 0x2000000
// "BSD class" prefix Darwin's amd64 syscall convention bakes into the
// number itself, and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so this
// is the one place that ever needs hand-written assembly for this
// (arch, OS) pair.
//
// The 0x2000000 prefix is the one concrete difference from
// linux/amd64's svc_amd64.s, confirmed empirically against real macOS
// execution while bringing up 6l's -H6 Mach-O target (see
// docs/claude_notes/notes_exec_macho.txt and
// tests/s/mini/hello_macos_amd64.s, which predates this file and is
// where this fact was first nailed down; arm64 has no such prefix, see
// svc_arm64.s in this same directory -- a genuine cross-arch asymmetry
// in how Darwin encodes the same BSD syscall numbers, not a copy-paste
// inconsistency). Everything else is unchanged from linux/amd64's
// version: no argument (not even num) arrives in a register -- this is
// an AMD64 SysV ABI fact, not an OS-specific one, so it applies
// identically regardless of GOOS -- the caller always writes every
// argument to the stack before CALL, letting this hand-written
// function read num+0(FP) directly with no register-vs-FP special
// case (see linux/amd64's svc_amd64.s for the full reasoning).
//
// XNU's error convention (carry flag set on return, AX holds the
// positive errno) differs from Linux's (negative errno already packed
// into the return value, no flag to check) -- this WAS unhandled here
// (see git history), harmless for write/exit since nothing ever
// inspected their return value, but open()/read() are exactly the
// "syscall a caller actually needs to detect failure from" this
// comment used to warn about. JCC/NEGQ below normalize XNU's convention
// to Linux's: success falls through with AX untouched, failure negates
// AX so callers everywhere can use the one `ret < 0` => -errno check.
// This is the minimum fix needed to make _syscall6's return value mean
// anything at all on error, hence living here in the raw trampoline
// rather than in os/darwin/'s Plan9-API-shape glue (see that
// directory's own header comment for the syscall/ vs os/ split).
// Not yet exercised against a real XNU failure (e.g. open() on a
// missing path) -- this host has no macOS available; verify on real
// hardware before trusting it fully (see docs/claude_notes/
// notes_libc_selfhost.txt).
TEXT _syscall6+0(SB), $0
	MOVQ	num+0(FP), AX
	ADDQ	$0x2000000, AX
	MOVQ	a1+8(FP), DI
	MOVQ	a2+16(FP), SI
	MOVQ	a3+24(FP), DX
	MOVQ	a4+32(FP), R10
	MOVQ	a5+40(FP), R8
	MOVQ	a6+48(FP), R9
	SYSCALL
	JCC	ok
	NEGQ	AX
ok:
	RET
