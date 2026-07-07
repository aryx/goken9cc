// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Mach-O executable writing, for ?l targeting macOS.
// Adapted from src/cmd/ld/macho.h (which supported only 386/amd64)
// for arm64 and for the simpler linkers/lk/ infrastructure
// (see elf.h for the ELF equivalent).

typedef struct MachoHdr MachoHdr;
struct MachoHdr {
	uint32	cpu;
	uint32	subcpu;
	uint32	flags;
};

typedef struct MachoSect MachoSect;
struct MachoSect {
	char*	name;
	uvlong	addr;
	uvlong	size;
	uint32	off;
	uint32	align;
	uint32	reloc;
	uint32	nreloc;
	uint32	flag;
};

typedef struct MachoSeg MachoSeg;
struct MachoSeg {
	char*	name;
	uvlong	vsize;
	uvlong	vaddr;
	uvlong	fileoffset;
	uvlong	filesize;
	uint32	prot1;	/* maxprot */
	uint32	prot2;	/* initprot */
	uint32	nsect;
	uint32	msect;
	MachoSect	*sect;
	uint32	flag;
};

typedef struct MachoLoad MachoLoad;
struct MachoLoad {
	uint32	type;
	uint32	ndata;
	uint32	*data;
};

/*
 * Total amount of space to reserve at the start of the file
 * for Header and load commands. May waste some.
 * The slack is also used by codesign(1) to insert its
 * LC_CODE_SIGNATURE load command after the fact.
 */
#define	MACHORESERVE	3*1024

enum {
	MACHO_CPU_AMD64 = (1<<24)|7,
	MACHO_CPU_386 = 7,
	MACHO_SUBCPU_X86 = 3,
	MACHO_CPU_ARM64 = (1<<24)|12,
	MACHO_SUBCPU_ARM64_ALL = 0,

	MACHO_EXECUTE = 2,		/* file type - mach executable */

	MACHO_NOUNDEFS = 1,		/* header flags */
	MACHO_DYLDLINK = 4,
	MACHO_TWOLEVEL = 0x80,
	MACHO_PIE = 0x200000,

	MACHO_SYMTAB = 2,		/* load commands */
	MACHO_UNIXTHREAD = 5,
	MACHO_DYSYMTAB = 11,
	MACHO_LOAD_DYLIB = 12,
	MACHO_LOAD_DYLINKER = 14,
	MACHO_SEGMENT_64 = 25,
	MACHO_BUILD_VERSION = 0x32,
	MACHO_MAIN = 0x80000028,
	MACHO_DYLD_INFO_ONLY = 0x80000022,

	/* rebase opcodes (high nibble) and immediates, for LC_DYLD_INFO */
	REBASE_TYPE_POINTER = 1,
	REBASE_OPCODE_DONE = 0x00,
	REBASE_OPCODE_SET_TYPE_IMM = 0x10,
	REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB = 0x20,
	REBASE_OPCODE_DO_REBASE_IMM_TIMES = 0x50,

	MACHO_PLATFORM_MACOS = 1,

	ARM_THREAD_STATE64 = 6,		/* thread state flavor */
	ARM_THREAD_STATE64_COUNT = 68,	/* x0-x28, fp, lr, sp, pc, cpsr (in 32-bit words) */
};

MachoHdr*	getMachoHdr(void);
MachoSeg*	newMachoSeg(char*, int);
MachoSect*	newMachoSect(MachoSeg*, char*);
MachoLoad*	newMachoLoad(uint32, uint32);
int	machowrite(void);
void	machorebase(void);
void	asmbmacho(void);
