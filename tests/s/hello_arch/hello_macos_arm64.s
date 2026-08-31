TEXT _start(SB), $0

    MOV    $setSB(SB), R28

    // macOS (XNU) syscall convention on arm64: the BSD syscall number
    // goes in R16 (not R8 as on Linux), arguments in R0-R2, and the
    // supervisor call immediate is 0x80 (SVC $0 would be a Mach trap).
    // See also hello_macos_amd64.s where the 0x2000000 "BSD class"
    // prefix plays the role that SVC $0x80 plays here.

    // write(int fd=1, buf=&msg, count=13)
    MOV    $1, R0          // fd = 1
    MOV    $msg(SB), R1    // buf = &msg
    MOV    $13, R2         // count = 13
    MOV    $4, R16         // syscall number: write
    SVC    $0x80

    // exit(int status=0)
    MOV    $0, R0          // status = 0
    MOV    $1, R16         // syscall number: exit
    SVC    $0x80

// -------------------------------------------
// msg: must split into 8-byte chunks
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"
GLOBL   msg(SB), $13
