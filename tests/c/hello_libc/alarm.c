#include <u.h>
#include <libc.h>

/* alarm() (include/os/time.h) -- the millisecond one-shot timer.
 *
 * Linux only for now, same scoping as notify.c next door and for the
 * same reason: the whole point of an alarm is the "alarm" note it
 * posts on expiry, and linux is the only GOOS here with a real
 * notify()/noted() to catch one with. plan9 has the syscall
 * (syscall/os/plan9/svc_$cputype.s) but 5i/vi implement neither it nor
 * note delivery; darwin has postnote() but no notify() yet; windows
 * returns a documented -1.
 *
 * Three separate claims, because two of them can pass by accident:
 *   1. the return value really is the time left on the PREVIOUS alarm
 *      (0 when none was pending) -- a stub returning 0 always would
 *      pass claim 3 alone;
 *   2. alarm(0) really cancels -- checked by arming a short alarm,
 *      cancelling it, and then sleeping well past when it would have
 *      fired;
 *   3. an alarm really does fire, and arrives as an "alarm" note
 *      through atnotify()'s dispatch chain.
 *
 * Timing margins are deliberately loose (a 100ms alarm given up to
 * ~3s to arrive) because these run under qemu-user for seven arches,
 * where wall-clock ratios are not the host's.
 */

int gotalarm;

int
handler(void *v, char *s)
{
	USED(v);
	if(strcmp(s, "alarm") == 0){
		gotalarm = 1;
		return 1;
	}
	return 0;
}

void
main(void)
{
	long prev, rem;
	int i;

	atnotify(handler, 1);

	/* claim 1: nothing pending yet, so this reports 0, and it reports
	 * the ~5s it just armed when immediately asked again */
	prev = alarm(5000);
	if(prev != 0){
		print("BUG: alarm() with none pending returned %ld, want 0\n", prev);
		exit(1);
	}
	rem = alarm(0);
	if(rem <= 0 || rem > 5000){
		print("BUG: remaining time was %ld, want 0 < t <= 5000\n", rem);
		exit(1);
	}

	/* claim 2: that alarm(0) really cancelled -- if it did not, the
	 * 5s alarm above is still counting and nothing below would be
	 * trustworthy. Re-arm something short, cancel it, wait past it. */
	alarm(100);
	if(alarm(0) <= 0){
		print("BUG: short alarm was not pending when cancelled\n");
		exit(1);
	}
	sleep(600);
	if(gotalarm){
		print("BUG: cancelled alarm fired anyway\n");
		exit(1);
	}

	/* claim 3: a real one arrives, as a real note */
	alarm(100);
	for(i = 0; i < 30 && !gotalarm; i++)
		sleep(100);
	if(!gotalarm){
		print("BUG: alarm note never delivered\n");
		exit(1);
	}

	print("alarm ok\n");
	exit(0);
}
