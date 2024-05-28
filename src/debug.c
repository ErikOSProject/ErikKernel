#include <debug.h>
#include <erikboot.h>
#include <memory.h>
#include <stdarg.h>

void putchar(char c)
{
	if (c == '\n')
		serial_putchar('\r');
	serial_putchar(c);
}

void print_int(unsigned long value, int base, int sign, int capital,
	       int padding, int precision, int leftJustified, int plusSigned,
	       int spaceSigned, int leadingZeros, int extraLeadingZero)
{
	char buffer[65];
	char base_char = capital ? 'A' : 'a';
	int negative = 0;

	if (base < 2 || base > 36)
		return;

	if (precision < 0)
		precision = 1;

	if (sign && (signed)value < 0) {
		negative = 1;
		value = (unsigned)(-(signed)value);
	}

	int index = 64;
	buffer[index] = 0;
	while (value > 0) {
		char d = value % base;
		buffer[--index] = d + ((d < 10) ? '0' : base_char - 10);
		value /= base;
	}

	if (extraLeadingZero) {
		buffer[--index] = '0';
	}

	if (negative)
		putchar('-');
	else if (plusSigned)
		putchar('+');
	else if (spaceSigned)
		putchar(' ');

	while (precision + index - 64 > 0)
		buffer[--index] = '0';

	if (!leftJustified)
		for (int i = 0; i < padding + index - 64; ++i)
			putchar(leadingZeros ? '0' : ' ');

	DEBUG_PRINT(buffer + index);

	if (leftJustified)
		for (int i = 0; i < padding + index - 64; ++i)
			putchar(leadingZeros ? '0' : ' ');
}

void print_float(double value, int base, int capital, int emode, int padding,
		 int precision, int leadingZeros, int alwaysPoint)
{
	int expo = 0;

	if (precision < 0)
		precision = 6;

	if (emode) {
		while (value < 1) {
			value *= base;
			--expo;
		}
		while (value >= base) {
			value /= base;
			++expo;
		}
	}

	if (base == 16)
		DEBUG_PRINT(capital ? "0X" : "0x");

	if (precision == 0)
		value += 0.5;
	int int_padding = padding - precision - (precision || alwaysPoint);
	print_int(value, base, 1, capital, int_padding, 0, 0, 0, 0,
		  leadingZeros, 0);
	value -= (long)value;

	if (precision || alwaysPoint)
		putchar('.');
	for (int i = 0; i < precision; ++i) {
		value *= base;
		long decPlace =
			(long)(value + (i >= precision - 1 ? 0.5 : 0.0));
		print_int(decPlace % base, base, 0, capital, 0, 1, 0, 0, 0, 1,
			  0);
	}

	if (emode) {
		if (base == 16)
			DEBUG_PRINT(capital ? "P" : "p");
		else
			DEBUG_PRINT(capital ? "E" : "e");
		print_int(expo, base, 1, capital, 0, 2, 0, 1, 0, 1, 0);
	}
}

void print_string(char *value, int precision, int padding, int leftJustified)
{
	size_t valueLen = strlen(value);
	if (!leftJustified && padding > 0)
		for (size_t i = 0; i < padding - valueLen; i++)
			putchar(' ');

	for (int i = 0; precision < 0 || i < precision; i++) {
		if (!value[i])
			break;

		putchar(value[i]);
	}

	if (leftJustified && padding > 0)
		for (size_t i = 0; i < padding - valueLen; i++)
			putchar(' ');
}

long lmod_value(char *lmod, va_list *args)
{
	long val;
	if (lmod[0] == 'h') {
		if (lmod[1] == 'h')
			val = (signed char)va_arg(*args, int);
		else
			val = (short)va_arg(*args, int);
	} else if (lmod[0] == 'l') {
		if (lmod[1] == 'l')
			val = va_arg(*args, long long);
		else
			val = va_arg(*args, long);
	} else if (lmod[0] == 'j')
		val = va_arg(*args, intmax_t);
	else if (lmod[0] == 'z')
		val = (signed)va_arg(*args, size_t);
	else if (lmod[0] == 't')
		val = va_arg(*args, ptrdiff_t);
	else
		val = va_arg(*args, int);
	return val;
}

void printf(const char *restrict format, ...)
{
	va_list args;
	va_start(args, format);
	int escaped = 0;

	for (; *format; format++)
		if (escaped) {
			int leftJustified = 0;
			int plusSigned = 0;
			int spaceSigned = 0;
			int altForm = 0;
			int leadingZeros = 0;
			while (1) {
				if (*format == '-')
					leftJustified = 1;
				else if (*format == '+')
					plusSigned = 1;
				else if (*format == ' ')
					spaceSigned = 1;
				else if (*format == '#')
					altForm = 1;
				else if (*format == '0')
					leadingZeros = 1;
				else
					break;
				format++;
			}

			int minSize = -1;
			if (*format >= '0' && *format <= '9')
				minSize = 0;
			while (*format >= '0' && *format <= '9') {
				minSize *= 10;
				minSize += *format - '0';
				format++;
			}

			if (*format == '*') {
				minSize = va_arg(args, int);
				format++;
			}

			int precision = -1;
			if (*format == '.') {
				precision = 0;
				format++;
				while (*format >= '0' && *format <= '9') {
					precision *= 10;
					precision += *format - '0';
					format++;
				}

				if (*format == '*') {
					precision = va_arg(args, int);
					format++;
				}
			}

			char lmod[2];
			if (*format == 'h') {
				lmod[0] = 'h';
				format++;
				if (*format == 'h') {
					lmod[1] = 'h';
					format++;
				}
			} else if (*format == 'l') {
				lmod[0] = 'l';
				format++;
				if (*format == 'l') {
					lmod[1] = 'l';
					format++;
				}
			} else if (*format == 'j') {
				lmod[0] = 'j';
				format++;
			} else if (*format == 'z') {
				lmod[0] = 'z';
				format++;
			} else if (*format == 't') {
				lmod[0] = 't';
				format++;
			} else if (*format == 'L') {
				lmod[0] = 'L';
				format++;
			}

			if (altForm) {
				if (*format == 'x')
					DEBUG_PRINT("0x");
				else if (*format == 'X')
					DEBUG_PRINT("0X");
			}

			if (*format == '%')
				putchar('%');
			else if (*format == 'c')
				putchar(va_arg(args, int));
			else if (*format == 's')
				print_string(va_arg(args, char *), precision,
					     minSize, leftJustified);
			else if (*format == 'd' || *format == 'i')
				print_int(lmod_value(lmod, &args), 10, 1, 0,
					  minSize, precision, leftJustified,
					  plusSigned, spaceSigned, leadingZeros,
					  0);
			else if (*format == 'u')
				print_int(lmod_value(lmod, &args), 10, 0, 0,
					  minSize, precision, leftJustified,
					  plusSigned, spaceSigned, leadingZeros,
					  0);
			else if (*format == 'o')
				print_int(lmod_value(lmod, &args), 8, 0, 0,
					  minSize, precision, leftJustified,
					  plusSigned, spaceSigned, leadingZeros,
					  altForm);
			else if (*format == 'x')
				print_int(lmod_value(lmod, &args), 16, 0, 0,
					  minSize, precision, leftJustified,
					  plusSigned, spaceSigned, leadingZeros,
					  0);
			else if (*format == 'X')
				print_int(lmod_value(lmod, &args), 16, 0, 1,
					  minSize, precision, leftJustified,
					  plusSigned, spaceSigned, leadingZeros,
					  0);
			else if (*format == 'f' || *format == 'F')
				print_float(va_arg(args, double), 10, 0, 0,
					    minSize, precision, leadingZeros,
					    altForm);
			else if (*format == 'e')
				print_float(va_arg(args, double), 10, 0, 1,
					    minSize, precision, leadingZeros,
					    altForm);
			else if (*format == 'E')
				print_float(va_arg(args, double), 10, 1, 1,
					    minSize, precision, leadingZeros,
					    altForm);
			else if (*format == 'a')
				print_float(va_arg(args, double), 16, 0, 1,
					    minSize, precision, leadingZeros,
					    altForm);
			else if (*format == 'A')
				print_float(va_arg(args, double), 16, 1, 1,
					    minSize, precision, leadingZeros,
					    altForm);
			else if (*format == 'g')
				print_float(va_arg(args, double), 10, 0, 0,
					    minSize, precision, leadingZeros,
					    1);
			else if (*format == 'G')
				print_float(va_arg(args, double), 10, 1, 0,
					    minSize, precision, leadingZeros,
					    1);
			else if (*format == 'p') {
				DEBUG_PRINT("0x");
				print_int((uintptr_t)va_arg(args, void *), 16, 0,
					  0, 0, 0, 0, 0, 0, 1, 0);
			}	else if (*format == 'n') {
				*va_arg(args, int *) = 0;
			} else
				putchar(*format);
			escaped = 0;
		} else if (*format == '%')
			escaped = 1;
		else
			putchar(*format);

	va_end(args);
}
