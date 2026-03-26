#include <stdio.h>
#include <stdarg.h>
#include "variadic_functions.h"

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

/**
 * print_char - prints a char argument from va_list
 * @args: pointer to va_list
 */
static void print_char(va_list *args)
{
	printf("%c", va_arg(*args, int));
}

/**
 * print_int - prints an int argument from va_list
 * @args: pointer to va_list
 */
static void print_int(va_list *args)
{
	printf("%d", va_arg(*args, int));
}

/**
 * print_float - prints a float argument from va_list
 * @args: pointer to va_list
 */
static void print_float(va_list *args)
{
	printf("%f", va_arg(*args, double));
}

/**
 * print_str - prints a string argument from va_list, or (nil) if NULL
 * @args: pointer to va_list
 */
static void print_str(va_list *args)
{
	const char *s = va_arg(*args, char *);

	if (s == NULL)
		s = "(nil)";
	printf("%s", s);
}

/**
 * print_all - prints anything based on a format string
 * @format: list of types: 'c'=char, 'i'=int, 'f'=float, 's'=char *
 *          any other character is ignored
 * @...: arguments to print
 */
void print_all(const char * const format, ...)
{
	static const fmt_t table[] = {
		{'c', print_char},
		{'i', print_int},
		{'f', print_float},
		{'s', print_str},
		{0, NULL}
	};
	va_list args;
	unsigned int i, j;
	int first = 1;

	va_start(args, format);
	i = 0;
	while (format && format[i])
	{
		j = 0;
		while (table[j].print && format[i] != table[j].spec)
			j++;

		if (table[j].print)
		{
			printf("%s", first ? "" : ", ");
			table[j].print(&args);
			first = 0;
		}
		i++;
	}
	va_end(args);
	printf("\n");
}
