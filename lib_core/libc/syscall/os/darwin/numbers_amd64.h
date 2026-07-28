/* Darwin (macOS) amd64 raw BSD syscall numbers -- same numbers as
 * darwin/arm64's (numbers_arm64.h): XNU's BSD syscall table is
 * arch-independent, only the trap encoding differs per arch (arm64:
 * SVC $0x80 with the number in R16; amd64: SYSCALL with the number in
 * AX plus a 0x2000000 "BSD class" prefix -- applied in svc_amd64.s,
 * not baked in here, so these stay the same bare numbers on both
 * arches). Confirmed empirically against real macOS execution while
 * bringing up 6l's -H6 Mach-O target (see
 * docs/claude_notes/notes_exec_macho.txt and
 * tests/s/mini/hello_macos_amd64.s, which predates this file).
 */

#define SYS_exit	1
#define SYS_write	4
