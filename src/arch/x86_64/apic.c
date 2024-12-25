/**
 * @file apic.c
 * @brief Advanced Programmable Interrupt Controller (APIC) functions.
 *
 * This file contains the implementation of functions related to the Advanced
 * Programmable Interrupt Controller (APIC) used in the x86_64 architecture.
 */

#include <acpi.h>
#include <arch.h>
#include <heap.h>
#include <memory.h>

#include <arch/x86_64/apic.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/syscall.h>

volatile uint32_t *lapic = NULL;

uint64_t lapic_ids[256];
uint8_t numcores = 0;

IOAPIC ioapics[256];
uint8_t numioapics = 0;

thread_info *core_bases;

/**
 * @brief Set GS for the current core.
 *
 * This function sets the GS register to the base address of the current core's
 * thread information structure.
 *
 * @param id The ID of the core.
 */
void set_core_base(uint64_t id)
{
	write_msr(MSR_GS_BASE, (uint64_t)&core_bases[id]);
	write_msr(MSR_KERNEL_GS_BASE, (uint64_t)&core_bases[id]);
	asm volatile("swapgs");
}

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
	load_gdt(id);
	load_idt();
	set_core_base(id);
	lapic[0x3c] = 0x1FF; // enable local APIC
	timer_init();
	asm volatile("sti");
	for (;;)
		asm volatile("hlt");
}

/**
 * @brief Initializes the Local APIC.
 *
 * This function initializes the Local APIC by finding the MADT table in the ACPI
 * tables and extracting the Local APIC address and IDs of the APs in the system.
 * It also discovers the IO APICs in the system.
 *
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 */
void apic_init(BootInfo *boot_info)
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
		// IO APIC
		else if (madt_entry[0] == 1) {
			uint32_t *ioapic =
				*(uint32_t **)((uintptr_t)madt_entry + 4);
			ioapics[numioapics].address = ioapic;
			ioapics[numioapics].base =
				*(uint32_t *)((uintptr_t)madt_entry + 8);
			ioapic[0] = 1;
			ioapics[numioapics].length = 1 + (ioapic[4] >> 16) &
						     0xFF;
			numioapics++;
		}
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
	core_bases = (thread_info *)malloc(sizeof(thread_info) * numcores);
	uintptr_t page = find_free_frames(numcores * 8);
	set_frame_lock(page, numcores * 8, true);
	ap_stacks = page + numcores * 0x800 - 1;
	for (size_t i = 0; i < numcores; i++) {
		core_bases[i].kernel_stack = page + i * 0x800 + 0x7ff;
		_gdt[i].tss.rsp[0] = page + i * 0x800 + 0x7ff;
	}
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

	set_core_base(0);
	timer_init();
}

/**
 * @brief Initializes the timer.
 *
 * This function initializes the Local APIC timer to generate periodic interrupts.
 *
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 */
void timer_init(void)
{
	lapic[APIC_DIV_TIMER] = 3; // divide by 16
	lapic[APIC_LVT_TIMER] = 0x20030; // periodic mode, vector 0x30
	lapic[APIC_TIMER_INITCNT] = 1000000;
}

/**
 * @brief Handles a timer tick.
 *
 * This function is called by the timer interrupt handler to handle a timer tick.
 */
void timer_tick(void)
{
	lapic[APIC_EOI] = 0;
}
