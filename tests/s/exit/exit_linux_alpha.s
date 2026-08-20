// run and then run 'echo $?' in your terminal
// and check that you get 42!

// this program does not require any .data segment
// so it should be simpler to assemble/link/run
// which can be useful when troubleshooting linker bugs

TEXT _start+0(SB), $0
    // Alpha Linux syscall ABI: v0/R0 holds the syscall number (and, on
    // return, the result); a0-a5/R16-R21 hold the arguments. CALL_PAL
    // $0x83 (aka "callsys") traps to the kernel. exit is syscall 1
    // (same number as x86, both inherited from old Unix numbering,
    // unlike most other Linux ports which renumber it).
    MOVQ    $42, R16       // a0 = exit code
    MOVQ    $1, R0         // v0 = syscall number = exit (1)
    CALL_PAL $0x83         // callsys
    RET                    // not reached
