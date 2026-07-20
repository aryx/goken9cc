    .global _start

    .section .data
msg:
    .asciz "Hello, world!\n"
msg_end:

    .equ msg_len, msg_end - msg

    .section .text
_start:
    // write(int fd, const void *buf, size_t count)
    mov     x8, #64         // syscall number: sys_write
    mov     x0, #1          // fd = 1 (stdout)
    ldr     x1, =msg        // buf = &msg
    mov     x2, #msg_len    // count = len
    svc     #0              // invoke syscall

    // exit(int status)
    mov     x8, #93         // syscall number: sys_exit
    mov     x0, #0          // status = 0
    svc     #0
