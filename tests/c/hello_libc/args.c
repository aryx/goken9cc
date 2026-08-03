#include <u.h>
#include <libc.h>

// claude: regression test for lib_core/libc/arch/$cputype/rt0.s's
// argc/argv bridge (every arch's _main used to just CALL/BL main with
// no argument setup at all -- see docs/claude_notes/notes_debug_
// techniques.txt's "Cross-arch rt0.s bring-up" section). argv[0] is
// deliberately not printed: it is the test binary's own path, which
// differs across environments (qemu vs native, different tmp dirs),
// so only argc and the fixed extra arguments the mkfile always passes
// are checked.
void
main(int argc, char *argv[])
{
	int i;

	print("argc=%d\n", argc);
	for(i = 1; i < argc; i++)
		print("argv[%d]=%s\n", i, argv[i]);
	exits(nil);
}
