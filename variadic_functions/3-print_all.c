#include <stdio.h>
#include "variadic_functions.h"

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
    const char *nil = "(nil)";
    const char *ptrs[2];

    ptrs[0] = nil;
    ptrs[1] = s;
    printf("%s", ptrs[s != NULL]);
}

/**
 * print_all - prints anything based on a format string
 * @format: list of types: 'c'=char, 'i'=int, 'f'=float, 's'=char *
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
    static const char * const sep[2] = {"", ", "};
    va_list args;
    unsigned int i = 0;
    unsigned int j;
    int first = 1;

    va_start(args, format);

    while (format && format[i])
    {
        j = 0;
        while (table[j].print)
        {
            if (format[i] == table[j].spec)
            {
                printf("%s", sep[!first]);
                table[j].print(&args);
                first = 0;
                break;
            }
            j++;
        }
        i++;
    }

    va_end(args);
    printf("\n");
}
