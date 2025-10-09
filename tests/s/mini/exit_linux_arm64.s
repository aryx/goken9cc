// run and then run 'echo $?' in your terminal
// and check you get 42!

// this program does not require any .data segment
// so it should be simpler to assemble/link/run
// which can be useful when troubleshooting linker bugs

TEXT _start(SB), $0
    // exit(int status)
    MOV    $42, R0         // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC    $0
    MOV    $12, R0

//TEXT _start(SB), 0, $0
//    ADR mydata, R1       // load address of .data
//    MOV (R1), R0         // read first word
//    MOV $93, R8           // exit
//    SVC $0
//mydata:
//    WORD $41
//    WORD $42
//    WORD $43

//TEXT _start(SB), 0, $0
//    MOV msg(SB), R0
//    MOV $93, R8           // exit
//    SVC $0
//
//DATA    msg+0(SB)/8, $"Hello, w"
//DATA    msg+8(SB)/6, $"orld\n"
//GLOBL   msg(SB), $14
