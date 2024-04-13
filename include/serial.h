#ifndef _SERIAL_H
#define _SERIAL_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	int (*init)(void *data);
	int (*reset)(void *data);
	void (*send)(void *data, char c);
} serial_driver;

typedef struct {
	serial_driver *driver;
	void *data;
} serial_device;

void serial_init(void);
void serial_putchar(char c);
void serial_print(const char *str);

#endif //_SERIAL_H
