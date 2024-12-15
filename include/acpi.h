/**
 * @file acpi.h
 * @brief ACPI helper functions.
 *
 * This file contains the declarations for the ACPI helper functions.
 */

#ifndef _ACPI_H
#define _ACPI_H

#include <erikboot.h>

#define EFI_ACPI_TABLE_GUID \
	{ 0x8868e871,       \
	  0xe4f1,           \
	  0x11d3,           \
	  { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

typedef struct {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
	uint32_t Length;
	uint64_t XsdtAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed)) XSDP;

typedef struct {
	char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
} ACPISDTHeader;

XSDP *find_xsdp(BootInfo *boot_info);
ACPISDTHeader *find_acpi_table(const char *signature, BootInfo *boot_info);
void smp_init(BootInfo *boot_info);

#endif //_ACPI_H
