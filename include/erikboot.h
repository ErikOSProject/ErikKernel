/**
 * @file erikboot.h
 * @brief Boot information structure.
 *
 * This file contains the definition of the boot information structure
 * that is passed to the kernel by the ErikBoot bootloader.
 */

#ifndef _ERIKBOOT_H
#define _ERIKBOOT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint32_t Type;
	uintptr_t PhysicalStart;
	uintptr_t VirtualStart;
	uint64_t NumberOfPages;
	uint64_t Attribute;
} MMapEntry;

typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} EFI_GUID;

typedef struct {
	EFI_GUID VendorGuid;
	void *VendorTable;
} EFIConfigurationTable;

typedef struct {
	void *FBBase;
	size_t FBSize;
	unsigned int FBWidth;
	unsigned int FBHeight;
	unsigned int FBPixelsPerScanLine;
	MMapEntry *MMapBase;
	size_t MMapEntryCount;
	size_t MMapEntrySize;
	char *InitrdBase;
	size_t InitrdSize;
	EFIConfigurationTable *EFIConfigurationTableBase;
	size_t EFIConfigurationTableEntryCount;
} BootInfo;

#endif //_ERIKBOOT_H
