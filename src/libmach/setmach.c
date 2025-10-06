#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<mach.h>
		/* table for selecting machine-dependent parameters */

typedef	struct machtab Machtab;

struct machtab
{
	char		*name;			/* machine name */
	short		type;			/* executable type */
	short		boottype;		/* bootable type */
	int		asstype;		/* disassembler code */
	Mach		*mach;			/* machine description */
	Machdata	*machdata;		/* machine functions */
};

extern	Mach		    mi386, mamd64, marm, marm64;
// no amd64mach
extern	Machdata		i386mach, armmach, arm64mach;

/*
 *	machine selection table.  machines with native disassemblers should
 *	follow the plan 9 variant in the table; native modes are selectable
 *	only by name.
 */
Machtab	machines[] =
{
	{	"386",				/*plan 9 386*/
		FI386,
		FI386B,
		AI386,
		&mi386,
		&i386mach,	},
	{	"amd64",			/*amd64*/
		FAMD64,
		FAMD64B,
		AAMD64,
		&mamd64,
		&i386mach,	}, // no amd64mach, reuse i386mach
	{	"arm",				/*ARM*/
		FARM,
		FARMB,
		AARM,
		&marm,
		&armmach,	},
    {	"arm64",			/*ARM64*/
		FARM64,
		FARM64B,
		AARM64,
		&marm64,
		&arm64mach,	},
//	{	"mips",				/*plan 9 mips*/
//		FMIPS,
//		FMIPSB,
//		AMIPS,
//		&mmips,
//		&mipsmach, 	},
	{	0		},		/*the terminator*/
};

/*
 *	select a machine by executable file type
 */
void
machbytype(int type)
{
	Machtab *mp;

	for (mp = machines; mp->name; mp++){
		if (mp->type == type || mp->boottype == type) {
			asstype = mp->asstype;
			machdata = mp->machdata;
			break;
		}
	}
}
/*
 *	select a machine by name
 */
int
machbyname(char *name)
{
	Machtab *mp;

	if (!name) {
		asstype = AAMD64;
		machdata = &i386mach;
		mach = &mamd64;
		return 1;
	}
	for (mp = machines; mp->name; mp++){
		if (strcmp(mp->name, name) == 0) {
			asstype = mp->asstype;
			machdata = mp->machdata;
			mach = mp->mach;
			return 1;
		}
	}
	return 0;
}
