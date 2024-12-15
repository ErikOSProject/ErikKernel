/**
 * @file acpi.c
 *@brief ACPI helper functions.
 *
 * This file contains the implementation of the ACPI helper functions.
 */

#include <acpi.h>
#include <debug.h>
#include <memory.h>

const EFI_GUID ACPI_GUID = EFI_ACPI_TABLE_GUID;

/**
 * @brief Finds the Extended System Description Pointer (XSDP) in the ACPI tables.
 *
 * This function searches the ACPI tables for the Extended System Description Pointer (XSDP).
 * The XSDP is a structure that contains the address of the Extended System Description Table (XSDT).
 *
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 * @return A pointer to the XSDP structure if found, or NULL if not found.
 */
XSDP *find_xsdp(BootInfo *boot_info)
{
	static XSDP *xsdp = NULL;
	if (xsdp)
		return xsdp;

	for (size_t i = 0; i < boot_info->EFIConfigurationTableEntryCount;
	     i++) {
		EFIConfigurationTable table =
			boot_info->EFIConfigurationTableBase[i];
		if (memcmp(&table.VendorGuid, &ACPI_GUID, sizeof(EFI_GUID)) ==
		    0)
			return xsdp = (XSDP *)table.VendorTable;
	}
	return NULL;
}

/**
 * @brief Finds an ACPI table with the specified signature.
 *
 * This function searches the ACPI tables for a table with the specified signature.
 * The signature is a 4-character string that identifies the table type.
 *
 * @param signature The 4-character signature of the table to find.
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 * @return A pointer to the ACPI table header if found, or NULL if not found.
 */
ACPISDTHeader *find_acpi_table(const char *signature, BootInfo *boot_info)
{
	XSDP *xsdp = find_xsdp(boot_info);
	if (!xsdp)
		return NULL;

	ACPISDTHeader *header = (ACPISDTHeader *)xsdp->XsdtAddress;
	uint64_t *table_ptrs = (uint64_t *)((uintptr_t)xsdp->XsdtAddress +
					    sizeof(ACPISDTHeader));
	size_t entries =
		(header->Length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);

	for (size_t i = 0; i < entries; i++) {
		ACPISDTHeader *header = (ACPISDTHeader *)table_ptrs[i];
		if (memcmp(header->Signature, signature, 4) == 0)
			return header;
	}

	return NULL;
}
