// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// PE (Portable Executable) file writing
// http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx

#include <time.h>

#include "l.h"
#include "../ld/lib.h"
#include "../ld/pe.h"

// DOS stub that prints out
// "This program cannot be run in DOS mode."
static char dosstub[] =
{
	0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
	0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd,
	0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 0x54, 0x68,
	0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72,
	0x61, 0x6d, 0x20, 0x63, 0x61, 0x6e, 0x6e, 0x6f,
	0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e,
	0x20, 0x69, 0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20,
	0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a,
	0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int pe64;
static int nsect;
static int sect_virt_begin;
static int sect_raw_begin = PERESERVE;

static IMAGE_FILE_HEADER fh;
static IMAGE_OPTIONAL_HEADER oh;	// PE32 (filled always; source of truth)
static IMAGE_OPTIONAL_HEADER64 oh64;	// PE32+ (derived from oh when pe64)
static IMAGE_SECTION_HEADER sh[16];
static IMAGE_SECTION_HEADER *textsect, *datsect, *bsssect;

// Imports from kernel32.dll. The layout of the .idata section (and thus
// the address of each import's IAT slot) is computed early, in peimports()
// called from dope(), so that reloc() can resolve references to the
// __imp_<name> symbols we define at those slots. add_import_table() then
// just writes the bytes during asmbpe().
static char *importdllname = "kernel32.dll";
static struct importfunc {
	char	*name;
	uint32	thunkrva;	// RVA of this import's hint/name entry
} importfuncs[] = {
	{ "GetStdHandle", 0 },
	{ "WriteFile", 0 },
	{ "ExitProcess", 0 },
	{ 0, 0 }
};
static IMAGE_IMPORT_DESCRIPTOR importds[2];
static IMAGE_SECTION_HEADER *importsect;
static uint32 importsize;	// logical size of the .idata contents
static uint32 importiatrva;	// RVA where the IAT (FirstThunk array) begins

static IMAGE_SECTION_HEADER*
new_section(char *name, int size, int noraw)
{
	IMAGE_SECTION_HEADER *h;

	if(nsect == 16) {
		diag("too many sections");
		errorexit();
	}
	h = &sh[nsect++];
	strncpy((char*)h->Name, name, sizeof(h->Name));
	h->VirtualSize = size;
	if(!sect_virt_begin)
		sect_virt_begin = 0x1000;
	h->VirtualAddress = sect_virt_begin;
	sect_virt_begin = rnd(sect_virt_begin+size, 0x1000);
	if(!noraw) {
		h->SizeOfRawData = rnd(size, PEALIGN);
		h->PointerToRawData = sect_raw_begin;
		sect_raw_begin += h->SizeOfRawData;
	}
	return h;
}

void
peinit(void)
{
	switch(thechar) {
	// 64-bit architectures
	case '6':
		pe64 = 1;
		break;
	// 32-bit architectures
	default:
		break;
	}
}

static void
pewrite(void)
{
	int i, j;

	ewrite(cout, dosstub, sizeof dosstub);
	strnput("PE", 4);

	for (i=0; i<sizeof(fh); i++)
		cput(((char*)&fh)[i]);
	if(pe64) {
		for (i=0; i<sizeof(oh64); i++)
			cput(((char*)&oh64)[i]);
	} else {
		for (i=0; i<sizeof(oh); i++)
			cput(((char*)&oh)[i]);
	}
	for (i=0; i<nsect; i++)
		for (j=0; j<sizeof(sh[i]); j++)
			cput(((char*)&sh[i])[j]);
	strnput("", PERESERVE-0x400);
}

// peimports lays out the .idata section and defines an __imp_<name>
// linker symbol at each import's IAT slot. It runs from dope() (before
// reloc()) so that "CALL __imp_WriteFile(SB)" style references resolve
// to the IAT slot address. The bytes are written later by
// add_import_table() during asmbpe().
static void
peimports(void)
{
	struct importfunc *f;
	uint32 size, va;
	int thunksize, n;

	// PE32+ import address table entries are 8 bytes wide, not 4; the
	// hint/name entries an entry points at are RVAs and stay 4 bytes.
	thunksize = pe64 ? 8 : 4;

	size = 0;
	memset(importds, 0, sizeof(importds));
	size += sizeof(importds);
	importds[0].Name = size;
	size += strlen(importdllname) + 1;
	for(f=importfuncs; f->name; f++) {
		f->thunkrva = size;
		size += sizeof(uint16) + strlen(f->name) + 1;
	}
	importds[0].FirstThunk = size;
	importiatrva = size;
	for(f=importfuncs; f->name; f++)
		size += thunksize;
	size += thunksize;	// null terminator for the IAT
	importsize = size;

	importsect = new_section(".idata", size, 0);
	importsect->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA|
		IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;

	va = importsect->VirtualAddress;
	oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = va;
	oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = importsect->VirtualSize;

	importds[0].Name += va;
	importds[0].FirstThunk += va;
	for(f=importfuncs; f->name; f++)
		f->thunkrva += va;
	importiatrva += va;

	// Define __imp_<name> at the (image-absolute) IAT slot. The slot
	// holds the resolved function pointer once the loader binds it, so
	// "CALL __imp_<name>(SB)" is an indirect call through the thunk.
	n = 0;
	for(f=importfuncs; f->name; f++, n++) {
		Sym *s;
		s = lookup(smprint("__imp_%s", f->name), 0);
		s->type = SFIXED;
		s->value = PEBASE + importiatrva + n*thunksize;
		s->reachable = 1;
	}
}

void
dope(void)
{
	textsect = new_section(".text", textsize, 0);
	textsect->Characteristics = IMAGE_SCN_CNT_CODE|
		IMAGE_SCN_CNT_INITIALIZED_DATA|
		IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ;

	// Only emit .data/.bss when non-empty: a zero-size section does not
	// advance the virtual address cursor, so the following section would
	// end up with a duplicate VirtualAddress and the image would be
	// rejected by the loader ("Bad EXE format").
	if(segdata.filelen > 0) {
		datsect = new_section(".data", segdata.filelen, 0);
		datsect->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA|
			IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
		if(INITDAT != PEBASE+datsect->VirtualAddress)
			diag("INITDAT = %#llux, want %#llux", (vlong)INITDAT, (vlong)(PEBASE+datsect->VirtualAddress));
	}

	if(segdata.len - segdata.filelen > 0) {
		bsssect = new_section(".bss", segdata.len - segdata.filelen, 1);
		bsssect->Characteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA|
			IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
	}

	peimports();
}

static void
strput(char *s)
{
	while(*s)
		cput(*s++);
	cput('\0');
}

// add_import_table writes the .idata bytes laid out earlier by
// peimports(). It appends to the end of the file (the .idata raw data is
// the last section) and restores the file position.
static void
add_import_table(void)
{
	struct importfunc *f;
	IMAGE_IMPORT_DESCRIPTOR *d;
	vlong off;

	// Write at the section's raw-data offset, not the current EOF: with a
	// non-empty .data the file position here is not page-aligned, whereas
	// PointerToRawData is, so appending would leave the .idata bytes short
	// of where the section header points ("file probably truncated").
	off = seek(cout, 0, 1);
	seek(cout, importsect->PointerToRawData, 0);
	for(d=importds; ; d++) {
		lputl(d->OriginalFirstThunk);
		lputl(d->TimeDateStamp);
		lputl(d->ForwarderChain);
		lputl(d->Name);
		lputl(d->FirstThunk);
		if(!d->Name)
			break;
	}
	strput(importdllname);
	for(f=importfuncs; f->name; f++) {
		wputl(0);
		strput(f->name);
	}
	// import address table: each entry is an RVA to a hint/name entry,
	// 4 bytes for PE32 and 8 bytes for PE32+, terminated by a zero entry.
	for(f=importfuncs; f->name; f++) {
		lputl(f->thunkrva);
		if(pe64)
			lputl(0);
	}
	lputl(0);		// IAT null terminator (low half)
	if(pe64)
		lputl(0);	// IAT null terminator (high half)
	strnput("", importsect->SizeOfRawData - importsize);
	cflush();
	seek(cout, off, 0);
}

void
asmbpe(void)
{
	switch(thechar) {
	default:
		diag("unknown PE architecture");
		errorexit();
	case '6':
		fh.Machine = IMAGE_FILE_MACHINE_AMD64;
		break;
	case '8':
		fh.Machine = IMAGE_FILE_MACHINE_I386;
		break;
	}

	if(!debug['s']) {
		IMAGE_SECTION_HEADER *symsect;
		symsect = new_section(".symdat", 8+symsize+lcsize, 0);
		symsect->Characteristics = IMAGE_SCN_MEM_READ|
			IMAGE_SCN_CNT_INITIALIZED_DATA;
	}

	add_import_table();

	fh.NumberOfSections = nsect;
	fh.TimeDateStamp = time(0);
	fh.SizeOfOptionalHeader = pe64 ? sizeof(oh64) : sizeof(oh);
	fh.Characteristics = IMAGE_FILE_RELOCS_STRIPPED|
		IMAGE_FILE_EXECUTABLE_IMAGE|IMAGE_FILE_DEBUG_STRIPPED;
	if(pe64)
		fh.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
	else
		fh.Characteristics |= IMAGE_FILE_32BIT_MACHINE;

	// oh (PE32) is filled unconditionally; for PE32+ it is copied field
	// by field into oh64 below (ImageBase and the stack/heap sizes widen).
	oh.Magic = pe64 ? IMAGE_NT_OPTIONAL_HDR64_MAGIC : IMAGE_NT_OPTIONAL_HDR32_MAGIC;
	oh.MajorLinkerVersion = 1;
	oh.MinorLinkerVersion = 0;
	oh.SizeOfCode = textsect->SizeOfRawData;
	oh.SizeOfInitializedData = datsect ? datsect->SizeOfRawData : 0;
	oh.SizeOfUninitializedData = bsssect ? bsssect->SizeOfRawData : 0;
	oh.AddressOfEntryPoint = entryvalue()-PEBASE;
	oh.BaseOfCode = textsect->VirtualAddress;
	oh.BaseOfData = datsect ? datsect->VirtualAddress : 0;	// absent in PE32+

	oh.ImageBase = PEBASE;
	oh.SectionAlignment = 0x00001000;
	oh.FileAlignment = PEALIGN;
	// 64-bit Windows predates subsystem version 4.0; use 6.0 (Vista era)
	// for PE32+, keep 4.0 for the legacy PE32 (386) output.
	oh.MajorOperatingSystemVersion = pe64 ? 6 : 4;
	oh.MinorOperatingSystemVersion = 0;
	oh.MajorImageVersion = 1;
	oh.MinorImageVersion = 0;
	oh.MajorSubsystemVersion = pe64 ? 6 : 4;
	oh.MinorSubsystemVersion = 0;
	oh.SizeOfImage = sect_virt_begin;
	oh.SizeOfHeaders = PERESERVE;
	oh.Subsystem = 3;	// WINDOWS_CUI
	oh.SizeOfStackReserve = 0x00200000;
	oh.SizeOfStackCommit = 0x00001000;
	oh.SizeOfHeapReserve = 0x00100000;
	oh.SizeOfHeapCommit = 0x00001000;
	oh.NumberOfRvaAndSizes = 16;

	if(pe64) {
		oh64.Magic = oh.Magic;
		oh64.MajorLinkerVersion = oh.MajorLinkerVersion;
		oh64.MinorLinkerVersion = oh.MinorLinkerVersion;
		oh64.SizeOfCode = oh.SizeOfCode;
		oh64.SizeOfInitializedData = oh.SizeOfInitializedData;
		oh64.SizeOfUninitializedData = oh.SizeOfUninitializedData;
		oh64.AddressOfEntryPoint = oh.AddressOfEntryPoint;
		oh64.BaseOfCode = oh.BaseOfCode;
		oh64.ImageBase = oh.ImageBase;
		oh64.SectionAlignment = oh.SectionAlignment;
		oh64.FileAlignment = oh.FileAlignment;
		oh64.MajorOperatingSystemVersion = oh.MajorOperatingSystemVersion;
		oh64.MinorOperatingSystemVersion = oh.MinorOperatingSystemVersion;
		oh64.MajorImageVersion = oh.MajorImageVersion;
		oh64.MinorImageVersion = oh.MinorImageVersion;
		oh64.MajorSubsystemVersion = oh.MajorSubsystemVersion;
		oh64.MinorSubsystemVersion = oh.MinorSubsystemVersion;
		oh64.Win32VersionValue = oh.Win32VersionValue;
		oh64.SizeOfImage = oh.SizeOfImage;
		oh64.SizeOfHeaders = oh.SizeOfHeaders;
		oh64.CheckSum = oh.CheckSum;
		oh64.Subsystem = oh.Subsystem;
		oh64.DllCharacteristics = oh.DllCharacteristics;
		oh64.SizeOfStackReserve = oh.SizeOfStackReserve;
		oh64.SizeOfStackCommit = oh.SizeOfStackCommit;
		oh64.SizeOfHeapReserve = oh.SizeOfHeapReserve;
		oh64.SizeOfHeapCommit = oh.SizeOfHeapCommit;
		oh64.LoaderFlags = oh.LoaderFlags;
		oh64.NumberOfRvaAndSizes = oh.NumberOfRvaAndSizes;
		memmove(oh64.DataDirectory, oh.DataDirectory, sizeof(oh.DataDirectory));
	}

	pewrite();
}
