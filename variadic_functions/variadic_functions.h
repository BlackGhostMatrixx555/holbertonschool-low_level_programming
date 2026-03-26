#ifndef VARIADIC_FUNCTIONS_H
#define VARIADIC_FUNCTIONS_H

#include <stdarg.h>

/**
 * struct fmt_entry - maps a format character to a print function
 * @spec: the format character
 * @print: pointer to function that prints the corresponding va_arg value
 */
typedef struct fmt_entry
{
	char spec;
	void (*print)(va_list *);
} fmt_t;

int sum_them_all(const unsigned int n, ...);
void print_numbers(const char *separator, const unsigned int n, ...);
void print_strings(const char *separator, const unsigned int n, ...);
void print_all(const char * const format, ...);

int _putchar(char c);

#endif /* VARIADIC_FUNCTIONS_H */
