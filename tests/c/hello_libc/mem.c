#include <u.h>
#include <libc.h>

/* sbrk() -- Tier 1 of docs/claude_notes/plan_syscalls.txt, and the one
 * call that actually blocks something: port/minimal_malloc.c cannot be
 * replaced by principia's real pool.c until libc can grow the heap.
 *
 * Underneath it, brk() is a different thing on each GOOS -- the kernel's
 * own already-right-shaped syscall on plan9, a syscall plus a
 * return-convention bridge on linux (os/linux/brk.c), and on darwin no
 * brk at all, just an mmap-backed sbrk (os/darwin/sbrk.c). This test is
 * deliberately written against sbrk() only, since that is the layer all
 * three agree on and the layer the toolchain's own callers use.
 *
 * What it checks, and why each check earns its place: last session's
 * four syscall bugs all hid in a return value nothing inspected, and
 * zero turned out to be the worst possible probe (seek(fd,0,0) survived
 * being mis-slotted, truncated AND dropped). So nothing here settles for
 * "it didn't crash": every sbrk return is inspected, the memory is
 * actually written and read back, and the two blocks are checked against
 * each other rather than in isolation -- an sbrk that returned the same
 * address twice would pass every single-block check.
 *
 * What is deliberately NOT here, against that same instinct, is a
 * failing sbrk. It is not portably assertable: qemu-user emulates brk
 * ITSELF rather than passing it to the host kernel, and accepts requests
 * a real kernel refuses. Measured, not assumed -- _sysbrk((void*)-1)
 * returned 0xFFFFFFFF on 386 as though it had succeeded, and segfaulted
 * inside the emulator on amd64/arm64. So the OS cannot be relied on to
 * reject anything here, and asserting a specific failure would be
 * asserting qemu's behavior rather than libc's. port/sbrk.c instead
 * guards the one case it can decide for itself (the bl+n overflow), and
 * that guard is real: it turns sbrk((ulong)-1) into -1 on 386 and riscv,
 * verified directly. See docs/claude_notes/notes_test_infra.txt.
 */

enum
{
	N1	= 100,
	N2	= 200
};

void
main(void)
{
	char *p1, *p2;
	int i;

	/* 1: a plain successful sbrk, and the memory it hands back is real.
	 * Writing then reading back matters: a broken brk() that moved the
	 * break too little (or not at all) still returns a plausible-looking
	 * pointer -- into unmapped memory. The pattern is i+1, not i, so a
	 * block of zeros can't pass for a correctly written one.
	 */
	p1 = sbrk(N1);
	if (p1 == (char*)-1) {
		print("sbrk(N1) failed\n");
		exit(1);
	}
	for (i = 0; i < N1; i++)
		p1[i] = (char)(i + 1);
	for (i = 0; i < N1; i++) {
		if (p1[i] != (char)(i + 1)) {
			print("first block did not read back\n");
			exit(1);
		}
	}

	/* 2: a second sbrk must hand back DIFFERENT memory that does not
	 * overlap the first. This is the check that a bump allocator can
	 * actually fail -- forgetting to advance `bloc` (port/sbrk.c) hands
	 * the same address out twice, and every check in step 1 would still
	 * pass.
	 */
	p2 = sbrk(N2);
	if (p2 == (char*)-1) {
		print("sbrk(N2) failed\n");
		exit(1);
	}
	if (p2 == p1) {
		print("sbrk returned the same address twice\n");
		exit(1);
	}
	if (p2 < p1 + N1) {
		print("second block overlaps the first\n");
		exit(1);
	}
	for (i = 0; i < N2; i++)
		p2[i] = (char)(0x55);

	/* 3: and writing all over the second block must not have disturbed
	 * the first. Overlap is already excluded by the pointer comparison
	 * above, but only for the START of p2 -- this catches a length that
	 * was mis-rounded or ignored.
	 */
	for (i = 0; i < N1; i++) {
		if (p1[i] != (char)(i + 1)) {
			print("second block clobbered the first\n");
			exit(1);
		}
	}

	print("sbrk ok\n");
	exit(0);
}
