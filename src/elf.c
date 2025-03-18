#include <elf.h>
#include <debug.h>
#include <heap.h>
#include <memory.h>
#include <paging.h>

/**
 * @brief Validates an ELF header.
 *
 * This function validates an ELF header to ensure that it is a valid ELF file.
 *
 * @param hdr A pointer to the ELF header structure.
 * @return true if the ELF header is valid, false otherwise.
 */
bool validate_elf(ELF_HEADER *hdr)
{
	uint8_t magic[4] = ELF_MAGIC;
	for (uint8_t i = 0; i < 4; i++)
		if (hdr->Id.Magic[i] != magic[i])
			return false;

	if (hdr->Id.ABI != 0)
		return false;

	if (hdr->Type != ELF_EXECUTABLE)
		return false;

	return true;
}

/**
 * @brief Loads an ELF file into memory.
 *
 * This function loads an ELF file into memory by mapping the program headers
 * into the process's address space and copying the program data into memory.
 *
 * @param node A pointer to the filesystem node containing the ELF file.
 * @param proc A pointer to the process structure.
 * @return true on success, false on failure.
 */
bool load_elf(fs_node *node, struct process *proc)
{
	ELF_HEADER hdr;
	bool ret = false;
	fs_read(node, (char *)&hdr, sizeof(ELF_HEADER));

	if (!validate_elf(&hdr))
		return false;

	ELF_PROGRAM_HEADER *phdrs;
	size_t pos = fs_seek(node, hdr.PHeaderBase, SEEK_SET);
	if (pos != hdr.PHeaderBase)
		return false;

	phdrs = malloc(hdr.PHeaderEntrySize * hdr.PHeaderEntryCount);
	if (fs_read(node, (char *)phdrs,
		    hdr.PHeaderEntrySize * hdr.PHeaderEntryCount) < 0)
		return false;

	uintptr_t highest = 0;
	uintptr_t lowest = SIZE_MAX;

	ELF_PROGRAM_HEADER *phdr = phdrs;
	for (size_t i = 0; i < hdr.PHeaderEntryCount; i++) {
		if (phdr->Type == ELF_P_LOAD) {
			size_t memory_pages =
				(phdr->MemorySize + PAGE_SIZE - 1) / PAGE_SIZE;
			intptr_t pages = find_free_frames(memory_pages);
			if (pages == -1)
				break;
			if (set_frame_lock(pages, memory_pages, true) < 0)
				break;
			paging_map_page(tables, phdr->VirtualStart,
					pages * PAGE_SIZE, P_USER_WRITE);
			paging_map_page(proc->tables, phdr->VirtualStart,
					pages * PAGE_SIZE, P_USER_WRITE);

			fs_seek(node, phdr->Offset, SEEK_SET);
			if (fs_read(node, (char *)phdr->VirtualStart,
				    phdr->FileSize) < 0)
				break;
		}
		phdr = (ELF_PROGRAM_HEADER *)((uintptr_t)phdr +
					      hdr.PHeaderEntrySize);
		if (i == hdr.PHeaderEntryCount)
			ret = true;
	}

	proc->image = malloc(sizeof(struct elf_image));
	proc->image->refcount = 1;
	proc->image->entry = hdr.Entry;
	proc->image->phsize = hdr.PHeaderEntrySize;
	proc->image->phnum = hdr.PHeaderEntryCount;
	proc->image->phdr = phdrs;

	return ret;
}
