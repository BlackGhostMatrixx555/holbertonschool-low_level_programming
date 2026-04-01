#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"

/*
 * Deterministic command-driven program for automated checker scenarios.
 *
 * Commands (one per line):
 *   ADD <id> <uid> <data>
 *   GET <id>
 *   SET <id> <data>
 *   DEL <id>
 *   CLEAR
 *   EXIT
 *
 * Output:
 *   OK / NO / FOUND / NOTFOUND + optional info
 */

static void trim_newline(char *s)
{
	size_t n;

	if (!s)
		return;

	n = strlen(s);
	if (n > 0 && s[n - 1] == '\n')
		s[n - 1] = '\0';
}

static void cmd_add(store_t *st, char *id, char *uid_s, char *data_s)
{
	unsigned int uid;
	session_t *s;

	if (!id || !uid_s || !data_s) {
		puts("NO");
		return;
	}

	uid = (unsigned int)strtoul(uid_s, NULL, 10);

	/* id/data come from the input buffer => lifetime ends after loop iteration. */
	s = session_create(id, uid, (const unsigned char *)data_s, strlen(data_s));
	if (!s) {
		puts("NO");
		return;
	}

	if (!store_add(st, s)) {
		puts("NO");
		return;
	}

	puts("OK");
}

static void cmd_get(store_t *st, char *id)
{
	session_t *s;

	if (!id) {
		puts("NOTFOUND");
		return;
	}

	s = store_get(st, id);
	if (!s) {
		puts("NOTFOUND");
		return;
	}

	printf("FOUND %s %u %lu\n",
		s->id ? s->id : "(null)",
		s->uid,
		(unsigned long)s->data_len);
}

static void cmd_set(store_t *st, char *id, char *data_s)
{
	session_t *s;

	if (!id || !data_s) {
		puts("NO");
		return;
	}

	s = store_get(st, id);
	if (!s) {
		puts("NO");
		return;
	}

	if (!session_set_data(s, (const unsigned char *)data_s, strlen(data_s))) {
		puts("NO");
		return;
	}

	puts("OK");
}

static void cmd_del(store_t *st, char *id)
{
	if (!id) {
		puts("NO");
		return;
	}

	/* Deletion in the CLI is delete-and-destroy: the store must free the session. */
	if (!store_delete(st, id, NULL)) {
		puts("NO");
		return;
	}

	puts("OK");
}

int main(void)
{
	store_t st;
	char line[512];

	store_init(&st);

	while (fgets(line, sizeof(line), stdin)) {
		char *cmd, *a, *b, *c;

		trim_newline(line);
		if (line[0] == '\0')
			continue;

		cmd = strtok(line, " ");
		if (!cmd)
			continue;

		if (strcmp(cmd, "ADD") == 0) {
			a = strtok(NULL, " ");
			b = strtok(NULL, " ");
			c = strtok(NULL, ""); /* rest of line as data */
			cmd_add(&st, a, b, c);
		} else if (strcmp(cmd, "GET") == 0) {
			a = strtok(NULL, " ");
			cmd_get(&st, a);
		} else if (strcmp(cmd, "SET") == 0) {
			a = strtok(NULL, " ");
			b = strtok(NULL, ""); /* rest of line as data */
			cmd_set(&st, a, b);
		} else if (strcmp(cmd, "DEL") == 0) {
			a = strtok(NULL, " ");
			cmd_del(&st, a);
		} else if (strcmp(cmd, "CLEAR") == 0) {
			store_destroy(&st);
			store_init(&st);
			puts("OK");
		} else if (strcmp(cmd, "EXIT") == 0) {
			break;
		} else {
			puts("NO");
		}
	}

	store_destroy(&st);
	return 0;
}