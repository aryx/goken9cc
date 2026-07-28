/* Darwin (macOS) arm64 raw BSD syscall numbers. Confirmed empirically
 * against real macOS execution while bringing up 7l's -H6 Mach-O
 * target (see tests/s/mini/hello_macos_arm64.s and
 * docs/claude_notes/notes_exec_macho.txt) before this file existed --
 * these are the same two numbers, just relocated here so libc.a's
 * generated syscall wrappers (zsyscall_darwin_arm64.c) can use them
 * too instead of every caller hand-writing the trap. Unlike Linux's
 * arm64 (asm-generic) numbers, these carry no "class" prefix: that's
 * an amd64/x86 Darwin quirk (the 0x2000000 BSD-class bit baked into
 * the syscall number itself, see numbers_amd64.h's own comment once
 * that arch is added) -- arm64 Darwin just wants the plain BSD number
 * in x16, no prefix.
 */

#define SYS_exit	1
#define SYS_write	4
