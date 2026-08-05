/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* STARTUPINFOA/PROCESS_INFORMATION byte layout, shared by
 * os/windows/exec.c and os/windows/spawn.c -- both build and pass
 * these two structs to CreateProcessA, and both need the same offsets.
 *
 * Unlike this project's earlier hand-derived guess (this file's own
 * git history), these are now computed from a real Windows SDK header
 * (MinGW-w64's processthreadsapi.h, found locally under a vendored
 * zig-codeberg toolchain checkout, not this project's own tree) rather
 * than derived field-by-field from memory:
 *
 *   struct _STARTUPINFOA {
 *     DWORD  cb;                 // offset 0,  size 4
 *     // 4 bytes padding to 8-byte-align the next field (a pointer)
 *     LPSTR  lpReserved;         // offset 8
 *     LPSTR  lpDesktop;          // offset 16
 *     LPSTR  lpTitle;            // offset 24
 *     DWORD  dwX;                // offset 32
 *     DWORD  dwY;                // offset 36
 *     DWORD  dwXSize;            // offset 40
 *     DWORD  dwYSize;            // offset 44
 *     DWORD  dwXCountChars;      // offset 48
 *     DWORD  dwYCountChars;      // offset 52
 *     DWORD  dwFillAttribute;    // offset 56
 *     DWORD  dwFlags;            // offset 60
 *     WORD   wShowWindow;        // offset 64
 *     WORD   cbReserved2;        // offset 66
 *     // 4 bytes padding to 8-byte-align the next field (a pointer)
 *     LPBYTE lpReserved2;        // offset 72
 *     HANDLE hStdInput;          // offset 80
 *     HANDLE hStdOutput;         // offset 88
 *     HANDLE hStdError;          // offset 96
 *   } STARTUPINFOA;              // total size 104
 *
 * STARTF_USESTDHANDLES (dwFlags bit, 0x100) is what makes hStdInput/
 * hStdOutput/hStdError actually take effect -- without it CreateProcessA
 * ignores all three and the child inherits this process's own console
 * handles instead, which is exactly what os/windows/exec.c's own
 * zeroed-flags stub already relied on (never redirects) and what
 * os/windows/spawn.c needs to turn ON for real pipe redirection.
 *
 *   struct _PROCESS_INFORMATION {
 *     HANDLE hProcess;   // offset 0
 *     HANDLE hThread;    // offset 8
 *     DWORD  dwProcessId;// offset 16
 *     DWORD  dwThreadId; // offset 20
 *   } PROCESS_INFORMATION; // total size 24
 *
 * Both buffers below are sized generously (128/32) rather than exactly
 * (104/24) -- same "too-large zeroed buffer is harmless, too-small is
 * a real stack overflow" asymmetry this project already applies
 * elsewhere for a struct with no real local header (os/linux/isatty.c's
 * struct termios buffer, this file's own earlier STARTUPINFOA guess).
 */
#define STARTUPINFOA_SIZE	128
#define STARTUPINFOA_CB		104
#define STARTUPINFOA_DWFLAGS_OFF	60
#define STARTUPINFOA_HSTDINPUT_OFF	80
#define STARTUPINFOA_HSTDOUTPUT_OFF	88
#define STARTUPINFOA_HSTDERROR_OFF	96
#define STARTF_USESTDHANDLES	0x00000100

#define PROCESS_INFORMATION_SIZE	32
#define PROCESS_INFORMATION_HPROCESS_OFF	0
