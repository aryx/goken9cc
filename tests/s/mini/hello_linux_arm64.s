TEXT _start(SB), 1, $0

    // seems to also work without the setSB MOV instruction, maybe
    // because MOV $msg(SB), ... further below can be encoded with the
    // direct virtual address instead of as an offset to SB (R28)
    MOV    $setSB(SB), R28
    // write(int fd=1, buf=&msg, count=14)
    MOV    $1, R0          // fd = 1
    MOV    $msg(SB), R1    // buf = &msg
//alt:   ADR msg, R1
    MOV    $14, R2         // count = 14
    MOV    $64, R8         // syscall number: write
    SVC     $0

    // exit(int status=0)
    MOV    $0, R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0

//alt:
// embed string directly in .text using WORD
// pack ASCII characters into 32-bit little-endian words
// useful to troubleshoot linking bugs by removing the
// need for a data segment
//msg:
//    WORD $0x6c6c6548     // Hell
//    WORD $0x57202c6f     // o, W
//    WORD $0x646c726f     // orld
//    WORD $0x0a21         // !\n (padded to 32-bit)

// -------------------------------------------
// msg: must split into 8-byte chunks
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/6, $"orld\n"
GLOBL   msg(SB), $14

//old: this was causing an error
//DATA msg+0(SB)/8, $"Hello, w"
//DATA msg+8(SB)/8, $"orld!\n\000"
//GLOBL msg(SB), $14
