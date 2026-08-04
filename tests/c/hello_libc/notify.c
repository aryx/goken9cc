#include <u.h>
#include <libc.h>

/* notify()/atnotify()/postnote()/noted() (include/os/plan9/note.h) --
 * Tier 6 notification (docs/claude_notes/plan_syscalls.txt). Linux
 * only for now: os/linux/notify.c gives notify()/noted() a real
 * implementation (sigaction()-based); the plan9 side (real syscalls)
 * exists too but machines/5i/vi can't actually deliver a note yet (a
 * documented emulator gap, notes_libc_selfhost.txt's own Tier 6
 * entry), so there is nothing runnable there to test.
 *
 * Exercises atnotify()'s real dispatch chain (not just the raw
 * notify() primitive) via postnote()-to-self, since that is the
 * layer every real caller in this tree (mk/Plan9.c, utilities/pipe/
 * tee.c, utilities/text/misc/ed.c) actually uses. Two notes handled
 * (proving noted(NCONT) really resumes execution, not just "the
 * process happened to survive"), one deliberately unhandled note
 * (proving a handler that returns 0 correctly falls through to the
 * next one, or to NDFLT if none matches -- here there is only the one
 * handler, so an unrecognized note it doesn't claim would fall
 * through to noted(NDFLT) and exit(1); not exercised here to keep the
 * test's own exit code meaningful), and one unrecognized postnote()
 * string (proving postnote() itself fails rather than guessing).
 */

int gotalarm;
int gothup;

int
handler(void *v, char *s)
{
	USED(v);
	if(strcmp(s, "alarm") == 0){
		gotalarm = 1;
		return 1;
	}
	if(strcmp(s, "hangup") == 0){
		gothup = 1;
		return 1;
	}
	return 0;
}

void
main(void)
{
	atnotify(handler, 1);

	if(postnote(PNPROC, getpid(), "alarm") != 0 || !gotalarm){
		print("BUG: alarm note not delivered\n");
		exit(1);
	}
	if(postnote(PNPROC, getpid(), "hangup") != 0 || !gothup){
		print("BUG: hangup note not delivered\n");
		exit(1);
	}
	/* still alive after two handled notes -- proves noted(NCONT)
	 * actually resumed execution */
	if(postnote(PNPROC, getpid(), "not a real note") == 0){
		print("BUG: postnote of an unrecognized note string succeeded\n");
		exit(1);
	}

	print("notify ok\n");
	exit(0);
}
