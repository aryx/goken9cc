// macOS (Darwin) arm64 hello world, written in Plan 9 7a syntax.
//
// Darwin's arm64 syscall convention differs from Linux's:
//   - the BSD syscall number goes in R16 (x16), not R8
//   - arguments are passed in R0..R5 as usual
//   - the trap is SVC $0x80 (the kernel reads the number from x16;
//     the immediate is conventionally 0x80 on Darwin)
//   - the numbers are the plain BSD numbers (write = 4, exit = 1);
//     the 0x2000000 "class" offset used on amd64 is an x86-only ABI detail.

TEXT _start(SB), $0

    MOV    $setSB(SB), R28      // set up SB; MOV $msg(SB) below may not need it

    // write(int fd=1, buf=&msg, count=13)
    MOV    $1, R0               // fd = 1 (stdout)
    MOV    $msg(SB), R1         // buf = &msg
    MOV    $13, R2              // count = 13
    MOV    $4, R16              // Darwin syscall number: write
    SVC    $0x80

    // exit(int status=0)
    MOV    $0, R0               // status = 0
    MOV    $1, R16              // Darwin syscall number: exit
    SVC    $0x80

// -------------------------------------------
// msg: must split into 8-byte chunks
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"
GLOBL   msg(SB), $13
