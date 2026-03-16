#include "main.h"
#include <stdlib.h>

/**
 * create_array - creates an array of chars and initializes it
 *                with a specific char
 * @size: size of the array to allocate
 * @c: char to initialize each element with
 *
 * Return: pointer to the allocated array,
 *         or NULL if size == 0 or allocation fails
 */
char *create_array(unsigned int size, char c)
{
	char *array;
	unsigned int i;

	if (size == 0)
		return (NULL);

	array = malloc(sizeof(char) * size);
	if (array == NULL)
		return (NULL);

	for (i = 0; i < size; i++)
		array[i] = c;

	return (array);
}
