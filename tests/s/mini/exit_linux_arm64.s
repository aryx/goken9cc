// run and then run 'echo $?' in your terminal
// and check you get 42!

// this program does not require any .data segment
// so it should be simpler to assembler/link/run
// which can be useful when troubleshooting linker bugs

TEXT _start(SB), 1, $0
    // exit(int status)
    MOV    $42, R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0
    MOV    $12, R0
