/**
 * @file msr.h
 * @brief Model Specific Registers for x86_64 architecture.
 *
 * This file contains the declarations for the Model Specific Registers for the x86_64 architecture.
 * It allows for reading and writing of MSRs.
 */

#ifndef _ARCH_MSR_H
#define _ARCH_MSR_H

#include <stdint.h>

#define MSR_EFER 0xC0000080
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_CSTAR 0xC0000083
#define MSR_SFMASK 0xC0000084
#define MSR_GS_BASE 0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102

#define EFER_SCE (1 << 0)

/**
 * @brief Read the value of a Model Specific Register.
 *
 * @param msr The MSR to read.
 * @return The value of the MSR.
 */
inline static uint64_t read_msr(uint32_t msr)
{
	uint32_t low, high;
	asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
	return ((uint64_t)high << 32) | low;
}

/**
 * @brief Write a value to a Model Specific Register.
 *
 * @param msr The MSR to write.
 * @param value The value to write to the MSR.
 */
inline static void write_msr(uint32_t msr, uint64_t value)
{
	uint32_t low = value & 0xFFFFFFFF;
	uint32_t high = value >> 32;
	asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

#endif //_ARCH_MSR_H
