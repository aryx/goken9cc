TEXT _start(SB), 1, $0

    // write(int fd=1, buf=&msg, count=14)
    MOV    $1, R0          // fd = 1
    MOV    $msg(SB), R1    // buf = &msg
    MOV    $14, R2         // count = 14
    MOV    $64, R8         // syscall number: write
    SVC     $0

    // exit(int status=0)
    MOV    $0, R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0

// -------------------------------------------
// msg: must split into 8-byte chunks
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/6, $"orld\n"
GLOBL   msg(SB), $14

// this was causing an error
//DATA msg+0(SB)/8, $"Hello, w"
//DATA msg+8(SB)/8, $"orld!\n\000"
//GLOBL msg(SB), $14
