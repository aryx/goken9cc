#include "gc.h"

/*
 * claude: matches every other arch's machcap() shape (a capability
 * query the frontend uses to ask "can you generate code for this
 * operation without help"); wasm has no arch-specific capability gaps
 * this bootstrap needs to report, so it's unconditionally "no".
 */
int
machcap(Node *n)
{
	USED(n);
	return 0;
}
