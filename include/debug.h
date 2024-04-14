#ifndef _DEBUG_H
#define _DEBUG_H

#include <stddef.h>
#include <stdint.h>

#ifdef DEBUG_PRINTK

#include <serial.h>

#define DEBUG_PRINT(str) serial_print(str)
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_INIT()                                  \
	{                                             \
		serial_init();                        \
		serial_print("\x1B[0m\x1B[2J\x1b[H"); \
	}

void printf(const char *, ...);

#else //DEBUG_PRINTK

#define DEBUG_PRINT(str) (void)str
#define DEBUG_PRINTF(...) (void)0
#define DEBUG_INIT()

#endif //DEBUG_PRINTK

#endif //_DEBUG_H
