#include "main.h"
#include <stdlib.h>

/**
 * _strdup - returns a pointer to a newly allocated space in memory,
 *           which contains a copy of the string given as a parameter
 * @str: string to duplicate
 *
 * Return: pointer to the duplicated string, or NULL if it fails
 */
char *_strdup(char *str)
{
	char *dup;
	unsigned int i, len;

	if (str == NULL)
		return (NULL);

	/* 1. Calculer la longueur de la chaîne d'origine */
	for (len = 0; str[len]; len++)
		;

	/* 2. Allouer la mémoire (taille + 1 pour le '\0') */
	dup = malloc(sizeof(char) * (len + 1));
	if (dup == NULL)
		return (NULL);

	/* 3. Copier chaque caractère */
	for (i = 0; i < len; i++)
		dup[i] = str[i];

	/* 4. Terminer par un '\0' */
	dup[len] = '\0';

	return (dup);
}
