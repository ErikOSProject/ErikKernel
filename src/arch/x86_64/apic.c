/**
 * @file apic.c
 * @brief Advanced Programmable Interrupt Controller (APIC) functions.
 *
 * This file contains the implementation of functions related to the Advanced
 * Programmable Interrupt Controller (APIC) used in the x86_64 architecture.
 */

#include <acpi.h>
#include <debug.h>
#include <memory.h>

#define APIC_ERROR 0xA0
#define APIC_ICR_LOW 0xC0
#define APIC_ICR_HIGH 0xC4

extern uintptr_t ap_entry;
extern uintptr_t ap_callback;
extern uintptr_t ap_stacks;

volatile uint32_t *lapic = NULL;
uint64_t lapic_ids[256];
uint8_t numcores = 0;
volatile bool ap_ready = false;

/**
 * @brief AP entry point.
 *
 * This function is the entry point for application processors (APs) in the system.
 * It is called by the AP bootstrap code to initialize the AP and start it running.
 *
 * @param id The ID of the AP.
 */
void test_ap(uint64_t id)
{
	DEBUG_PRINTF("Hello from AP %d!\n", id);
	ap_ready = true;
	for (;;)
		asm volatile("hlt");
}

/**
 * @brief Gets the Local APIC address from the MADT.
 *
 * This function searches the MADT table for the Local APIC address and stores
 * it in the lapic variable. It also retrieves the LAPIC IDs of all cores in the
 * system and stores them in the lapic_ids array.
 *
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 */
void get_lapic(BootInfo *boot_info)
{
	XSDP *xsdp = find_xsdp(boot_info);
	ACPISDTHeader *madt = find_acpi_table("APIC", boot_info);
	lapic = (uint32_t *)(*(uint64_t *)((uintptr_t)madt +
					   sizeof(ACPISDTHeader)) &
			     0xFFFFFFFF);

	uint8_t *madt_entry =
		(uint8_t *)((uintptr_t)madt + sizeof(ACPISDTHeader) + 8);
	while ((uintptr_t)madt_entry < (uintptr_t)madt + madt->Length) {
		// Processor Local APIC
		if (madt_entry[0] == 0)
			lapic_ids[numcores++] = madt_entry[2];
		madt_entry += madt_entry[1];
	}
}

/**
 * @brief Gets the BSP ID.
 *
 * This function retrieves the ID of the Bootstrap Processor (BSP) from the CPUID
 * instruction and returns it.
 *
 * @return The ID of the BSP.
 */
uint32_t get_bsp_id(void)
{
	uint32_t bspid;
	asm volatile("mov $1, %%eax; cpuid; shrl $24, %%ebx;"
		     : "=b"(bspid)
		     :
		     :);
	return bspid;
}

/**
 * @brief Allocates stacks for the application processors.
 */
void allocate_ap_stacks(void)
{
	uintptr_t page = find_free_frames((numcores - 1) * 8);
	set_frame_lock(page, (numcores - 1) * 8, true);
	ap_stacks = page + (numcores - 1) * 0x800 - 1;
}

/**
 * @brief Relocates the AP trampoline code.
 */
void relocate_ap_trampoline(void)
{
	ap_callback = (uintptr_t)test_ap;
	memcpy((void *)0x8000, (char *)&ap_entry, 4096);
}

/**
 * @brief Starts an application processor.
 *
 * This function sends INIT and STARTUP IPIs to the specified AP and waits for
 * the AP to signal readiness before returning.
 *
 * @param id The ID of the AP to start.
 */
void start_ap(uint8_t id)
{
	ap_ready = false;
	// send INIT IPI
	lapic[APIC_ERROR] = 0;
	lapic[APIC_ICR_HIGH] = (lapic_ids[id] << 24);
	lapic[APIC_ICR_LOW] = 0xc500;
	// wait for pending status to clear
	do {
		__asm__ __volatile__("pause" : : : "memory");
	} while (lapic[APIC_ICR_LOW] & (1 << 12));
	// deassert INIT IPI
	lapic[APIC_ICR_HIGH] = (lapic_ids[id] << 24);
	lapic[APIC_ICR_LOW] = 0x8500;
	// wait for pending status to clear
	do {
		__asm__ __volatile__("pause" : : : "memory");
	} while (lapic[APIC_ICR_LOW] & (1 << 12));
	// send STARTUP IPI (twice)
	for (int j = 0; j < 2; j++) {
		lapic[APIC_ERROR] = 0;
		lapic[APIC_ICR_HIGH] = (lapic_ids[id] << 24);
		lapic[APIC_ICR_LOW] = 0x608;
		// wait for pending status to clear
		do {
			__asm__ __volatile__("pause" : : : "memory");
		} while (lapic[APIC_ICR_LOW] & (1 << 12));
	}
	// wait for AP to signal readiness
	do {
		__asm__ __volatile__("pause" : : : "memory");
	} while (!ap_ready);
}

/**
 * @brief Initializes the Symmetric Multiprocessing (SMP) system.
 *
 * This function initializes the SMP system by enabling the Local APIC and starting
 * the application processors.
 *
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 */
void smp_init(BootInfo *)
{
	lapic[0x3c] = 0x1FF; // enable local APIC
	allocate_ap_stacks();
	relocate_ap_trampoline();

	uint32_t bspid = get_bsp_id();
	for (uint8_t i = 0; i < numcores; i++)
		if (lapic_ids[i] != bspid)
			start_ap(i);
}
