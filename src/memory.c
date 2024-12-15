/**
 * @file memory.c
 * @brief Memory management functions for ErikKernel.
 *
 * This file contains the implementation of memory management functions
 * used in the ErikKernel. These functions are responsible for managing
 * physical memory, allocating and freeing memory, and initializing the
 * page frame allocator.
 */

#include <erikboot.h>
#include <memory.h>
#include <debug.h>

#define EFI_CONVENTIONAL_MEMORY 7

memory _memory = { 0 };

/**
 * @brief Fills a block of memory with a specified value.
 *
 * This function sets the first `num` bytes of the block of memory 
 * pointed to by `destination` to the specified value `c`.
 *
 * @param destination Pointer to the block of memory to fill.
 * @param c Value to be set. The value is passed as an int, but the function 
 *         fills the block of memory using an 8-bit conversion of this value.
 * @param num Number of bytes to be set to the value.
 * @return A pointer to the memory area `destination`.
 */
void *memset(void *destination, int c, size_t num)
{
	for (size_t i = 0; i < num; i++)
		((uint8_t *)destination)[i] = (uint8_t)c;
	return destination;
}

/**
 * @brief Calculates the length of a null-terminated string.
 *
 * This function takes a pointer to a null-terminated string and returns
 * the number of characters in the string, excluding the null terminator.
 *
 * @param str Pointer to the null-terminated string.
 * @return The length of the string, excluding the null terminator.
 */
size_t strlen(const char *str)
{
	size_t len = 0;
	for (; *str; ++str)
		len++;
	return len;
}

/**
 * @brief Copies n bytes from memory area source to memory area destination.
 *
 * This function copies n bytes from the memory area pointed to by source
 * to the memory area pointed to by destination. The memory areas must not
 * overlap, as the behavior is undefined if they do.
 *
 * @param destination Pointer to the destination memory area.
 * @param source Pointer to the source memory area.
 * @param n Number of bytes to copy.
 * @return A pointer to the destination memory area.
 */
char *memcpy(char *destination, const char *source, size_t n)
{
	for (size_t i = 0; i < n; i++)
		destination[i] = source[i];
	return destination;
}

/**
 * @brief Copies the string pointed to by source (including the null terminator) 
 *        to the array pointed to by destination.
 *
 * @param destination Pointer to the destination array where the content is to be copied.
 * @param source Pointer to the null-terminated string to be copied.
 * @return A pointer to the destination string.
 */
char *strcpy(char *destination, const char *source)
{
	size_t i = 0;
	for (; source[i]; i++)
		destination[i] = source[i];
	destination[i] = 0;
	return destination;
}

/**
 * @brief Concatenates the source string to the destination string.
 *
 * This function appends the source string to the destination string,
 * overwriting the null byte ('\0') at the end of destination, and then
 * adds a terminating null byte. The destination string must have enough
 * space to hold the result.
 *
 * @param destination Pointer to the destination array, which should contain
 *                    a C string, and be large enough to contain the concatenated
 *                    resulting string.
 * @param source      Pointer to the source C string to be appended.
 * @return A pointer to the destination string.
 */
char *strcat(char *destination, const char *source)
{
	char *new_dest = destination + strlen(destination);
	strcpy(new_dest, source);
	return destination;
}

/**
 * @brief Compares two blocks of memory.
 *
 * This function compares the first n bytes of the memory areas str1 and str2.
 *
 * @param str1 Pointer to the first memory block.
 * @param str2 Pointer to the second memory block.
 * @param n Number of bytes to compare.
 * @return An integer less than, equal to, or greater than zero if the first n bytes of str1 
 *         is found, respectively, to be less than, to match, or be greater than the first n bytes of str2.
 */
int memcmp(const void *str1, const void *str2, size_t n)
{
	size_t i = 0;
	while (((const char *)str1)[i] == ((const char *)str2)[i] && i < n)
		i++;
	return (i < n) ? ((const char *)str1)[i] - ((const char *)str2)[i] : 0;
}

/**
 * @brief Compares two null-terminated strings.
 *
 * This function compares the string pointed to by `str1` to the string pointed to by `str2`.
 * The comparison is done lexicographically using the unsigned char values of each character.
 *
 * @param str1 Pointer to the first null-terminated string to be compared.
 * @param str2 Pointer to the second null-terminated string to be compared.
 * @return An integer less than, equal to, or greater than zero if `str1` is found,
 *         respectively, to be less than, to match, or be greater than `str2`.
 */
int strcmp(const char *str1, const char *str2)
{
	size_t i = 0;
	while (str1[i] == str2[i] && str1[i] != 0)
		i++;
	return str1[i] - str2[i];
}

/**
 * @brief Splits a string into a sequence of tokens based on the specified delimiters.
 *
 * @param str The input string to be tokenized. If NULL, the function continues tokenizing the previous string.
 * @param delimiters A string containing the delimiter characters used to split the input string.
 * @return A pointer to the next token, or NULL if there are no more tokens.
 */
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

/**
 * @brief Fills a region of a bitmap with a specified value.
 *
 * @param bitmap Pointer to the bitmap array.
 * @param start_bit The starting bit position in the bitmap to begin filling.
 * @param num_bits The number of bits to fill in the bitmap.
 * @param value The value to set the bits to (0 or 1).
 */
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

/**
 * @brief Retrieves the size of the memory from the boot information.
 *
 * This function extracts and returns the size of the memory available
 * from the provided boot information structure.
 *
 * @param boot_info A pointer to the BootInfo structure containing
 *                  the boot information.
 */
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

/**
 * @brief Finds a contiguous block of free memory frames.
 *
 * This function searches for a contiguous block of free memory frames of the specified size.
 *
 * @param n The number of contiguous free frames required.
 * @return The starting address of the first free frame if a block of the required size is found, 
 *         or -1 if no such block is available.
 */
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

/**
 * @brief Sets the lock state for a range of memory frames.
 *
 * This function sets or clears the lock state for a specified number of memory frames
 * starting from a given frame address.
 *
 * @param frame The starting frame address.
 * @param n The number of frames to lock or unlock.
 * @param lock A boolean value indicating whether to lock (true) or unlock (false) the frames.
 * @return The starting frame address if the operation is successful, or -1 if the frame address is invalid.
 */
intptr_t set_frame_lock(uintptr_t frame, size_t n, bool lock)
{
	if (frame < _memory.base || frame > _memory.base + _memory.length)
		return -1;
	fill_bitmap_region(_memory.bitmap, (frame - _memory.base) / PAGE_SIZE,
			   n, lock);
	return frame;
}

/**
 * @brief Initializes the page frame allocator with the provided boot information.
 *
 * This function sets up the page frame allocator using the memory map and other
 * relevant information provided in the BootInfo structure. It prepares the system
 * for memory management by identifying available and reserved memory regions.
 *
 * @param boot_info A pointer to the BootInfo structure containing the memory map
 *                  and other boot-related information.
 */
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
