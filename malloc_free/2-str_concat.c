#include "main.h"
#include <stdlib.h>

/**
 * str_concat - concatenates two strings
 * @s1: first string
 * @s2: second string
 *
 * Return: pointer to newly allocated memory containing s1 + s2,
 * or NULL on failure
 */
char *str_concat(char *s1, char *s2)
{
	char *concat;
	unsigned int i, j, len1 = 0, len2 = 0;

	/* 1. Gérer les chaînes NULL */
	if (s1 == NULL)
		s1 = "";
	if (s2 == NULL)
		s2 = "";

	/* 2. Calculer la longueur de s1 et s2 */
	for (len1 = 0; s1[len1]; len1++)
		;
	for (len2 = 0; s2[len2]; len2++)
		;

	/* 3. Allouer la mémoire pour la concaténation */
	concat = malloc(sizeof(char) * (len1 + len2 + 1));
	if (concat == NULL)
		return (NULL);

	/* 4. Copier s1 dans la nouvelle mémoire */
	for (i = 0; i < len1; i++)
		concat[i] = s1[i];

	/* 5. Ajouter s2 à la suite */
	for (j = 0; j < len2; j++)
		concat[i + j] = s2[j];

	/* 6. Terminer par un '\0' */
	concat[i + j] = '\0';

	return (concat);
}
