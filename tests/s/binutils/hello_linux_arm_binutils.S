// arm-linux-gnueabihf-as -o hello.o hello_linux_arm.s
// arm-linux-gnueabihf-ld -o hello hello.o

    .global _start

    .section .data
msg:
    .asciz "Hello, world!\n"
msg_end:

    .equ msg_len, msg_end - msg

    .section .text
_start:
    @ write(int fd, const void *buf, size_t count)
    mov     r7, #4              @ syscall number: sys_write
    mov     r0, #1              @ fd = 1 (stdout)
    ldr     r1, =msg            @ buf = &msg
    ldr     r2, =msg_len        @ count = len
    svc     0

    @ exit(int status)
    mov     r7, #1              @ syscall number: sys_exit
    mov     r0, #0              @ status = 0
    svc     0
