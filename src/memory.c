#include <erikboot.h>
#include <memory.h>
#include <debug.h>

#define EFI_CONVENTIONAL_MEMORY 7

memory _memory = { 0 };

void *memset(void *destination, int c, size_t num)
{
	for (size_t i = 0; i < num; i++)
		((uint8_t *)destination)[i] = (uint8_t)c;
	return destination;
}

size_t strlen(const char *str)
{
	size_t len = 0;
	for (; *str; ++str)
		len++;
	return len;
}

char *memcpy(char *destination, const char *source, size_t n)
{
	for (size_t i = 0; i < n; i++)
		destination[i] = source[i];
	return destination;
}

char *strcpy(char *destination, const char *source)
{
	size_t i = 0;
	for (; source[i]; i++)
		destination[i] = source[i];
	destination[i] = 0;
	return destination;
}

char *strcat(char *destination, const char *source)
{
	char *new_dest = destination + strlen(destination);
	strcpy(new_dest, source);
	return destination;
}

int memcmp(const char *str1, const char *str2, size_t n)
{
	size_t i = 0;
	while (str1[i] == str2[i] && i < n)
		i++;
	return str1[i] - str2[i];
}

int strcmp(const char *str1, const char *str2)
{
	size_t i = 0;
	while (str1[i] == str2[i] && str1[i] != 0)
		i++;
	return str1[i] - str2[i];
}

char *strtok(char *str, const char *delimiters)
{
	static char *s;
	char *begin = NULL;
	if (str)
		s = str;

	while (*s) {
		bool d_found = false;
		for (const char *d = delimiters; *d; ++d)
			if (*s == *d) {
				d_found = true;
				break;
			}

		if (!begin && !d_found)
			begin = s;

		if (begin && d_found) {
			*(s++) = 0;
			break;
		}

		s++;
	}

	return begin;
}

void fill_bitmap_region(uint8_t *bitmap, unsigned int start_bit,
			size_t num_bits, bool value)
{
	unsigned int start_offset = start_bit % 8;
	unsigned int end_offset = (start_bit + num_bits) % 8;
	unsigned int first_full_byte = start_bit / 8 + (start_offset ? 1 : 0);
	unsigned int last_full_byte =
		(start_bit + num_bits) / 8 - (end_offset ? 0 : 1);

	if (start_offset) {
		uint8_t mask = (0xFF << start_offset) & 0xFF;
		if (first_full_byte - 1 == last_full_byte && end_offset)
			mask &= (0xFF >> (8 - end_offset));
		if (value)
			bitmap[first_full_byte - 1] |= mask;
		else
			bitmap[first_full_byte - 1] &= ~mask;
	}

	memset(&bitmap[first_full_byte], value ? 0xFF : 0x00,
	       last_full_byte - first_full_byte + 1);

	if (end_offset && (last_full_byte + 1 > first_full_byte)) {
		uint8_t mask = (0xFF >> (8 - end_offset)) & 0xFF;
		if (value)
			bitmap[last_full_byte + 1] |= mask;
		else
			bitmap[last_full_byte + 1] &= ~mask;
	}
}

void memory_get_size(BootInfo *boot_info)
{
	_memory.base = UINTPTR_MAX;
	_memory.length = 0;

	if (boot_info->MMapEntryCount == 0)
		return;

	MMapEntry *memory_map = boot_info->MMapBase;
	for (size_t i = 0; i < boot_info->MMapEntryCount; i++) {
		if (memory_map->Type == EFI_CONVENTIONAL_MEMORY &&
		    _memory.bitmap == 0)
			_memory.bitmap = (uint8_t *)memory_map->PhysicalStart;
		if (memory_map->PhysicalStart < _memory.base)
			_memory.base = memory_map->PhysicalStart;
		if ((memory_map->PhysicalStart +
		     memory_map->NumberOfPages * PAGE_SIZE) > _memory.length)
			_memory.length = memory_map->PhysicalStart +
					 memory_map->NumberOfPages * PAGE_SIZE;
		memory_map = (MMapEntry *)((uintptr_t)memory_map +
					   boot_info->MMapEntrySize);
	}

	_memory.length -= _memory.base;
}

intptr_t find_free_frames(size_t n)
{
	size_t count = 0;
	for (size_t i = _memory.base / PAGE_SIZE;
	     i < _memory.length / PAGE_SIZE; i++) {
		size_t byte_index = (i - _memory.base / PAGE_SIZE) / 8;
		size_t bit_index = i % 8;

		if (!(_memory.bitmap[byte_index] & (1 << bit_index)))
			count++;
		else
			count = 0;

		if (count == n)
			return (i - n + 1) * PAGE_SIZE;
	}
	return -1;
}

intptr_t set_frame_lock(uintptr_t frame, size_t n, bool lock)
{
	if (frame < _memory.base || frame > _memory.base + _memory.length)
		return -1;
	fill_bitmap_region(_memory.bitmap, (frame - _memory.base) / PAGE_SIZE,
			   n, lock);
	return frame;
}

void page_frame_allocator_init(BootInfo *boot_info)
{
	memory_get_size(boot_info);

	if (_memory.bitmap == 0) {
		DEBUG_PRINTF("Could not initialize pfa!\n");
		for (;;)
			;
	}

	size_t bitmap_length = _memory.length / PAGE_SIZE / 8;
	memset(_memory.bitmap, 0xFF, bitmap_length);

	MMapEntry *memory_map = boot_info->MMapBase;
	for (size_t i = 0; i < boot_info->MMapEntryCount; i++) {
		if (memory_map->Type == EFI_CONVENTIONAL_MEMORY)
			set_frame_lock(memory_map->PhysicalStart,
				       memory_map->NumberOfPages, false);

		memory_map = (MMapEntry *)((uintptr_t)memory_map +
					   boot_info->MMapEntrySize);
	}

	set_frame_lock((uintptr_t)_memory.bitmap, bitmap_length / PAGE_SIZE + 1,
		       true);
}
