#ifndef _ELF_H
#define _ELF_H

#include <erikboot.h>
#include <fs.h>
#include <task.h>

#define ELF_MAGIC "\177ELF"
#define ELF_EXECUTABLE 2
#define ELF_P_LOAD 1

typedef struct {
	uint8_t Magic[4];
	uint8_t Type;
	uint8_t Endianness;
	uint8_t Version;
	uint8_t ABI;
	uint8_t Unused[8];
} ELF_IDENTIFICATION;

typedef struct {
	ELF_IDENTIFICATION Id;
	uint16_t Type;
	uint16_t Machine;
	uint32_t Version;
	uint64_t Entry;
	uint64_t PHeaderBase;
	uint64_t SHeaderBase;
	uint32_t Flags;
	uint16_t HeaderSize;
	uint16_t PHeaderEntrySize;
	uint16_t PHeaderEntryCount;
	uint16_t SHeaderEntrySize;
	uint16_t SHeaderEntryCount;
	uint16_t SHeaderIndex;
} ELF_HEADER;

typedef struct {
	uint32_t Type;
	uint32_t Flags;
	uint64_t Offset;
	uint64_t VirtualStart;
	uint64_t Unused;
	uint64_t FileSize;
	uint64_t MemorySize;
} ELF_PROGRAM_HEADER;

struct elf_image {
	int refcount;
	uint64_t entry;
	uint16_t phsize;
	uint16_t phnum;
	ELF_PROGRAM_HEADER *phdr;
};

bool validate_elf(ELF_HEADER *hdr);
bool load_elf(fs_node *node, struct process *proc);

#endif //_ELF_H
