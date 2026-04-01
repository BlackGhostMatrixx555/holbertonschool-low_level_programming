#ifndef STORE_H
#define STORE_H

#include "session.h"

typedef struct node_s {
	session_t *sess;
	struct node_s *next;
} node_t;

typedef struct store_s {
	node_t *head;
} store_t;

void store_init(store_t *st);
int store_add(store_t *st, session_t *s);
session_t *store_get(store_t *st, const char *id);

/*
 * Deletes a session by id.
 * Returns 1 if deleted, 0 if not found.
 */
int store_delete(store_t *st, const char *id, session_t **out);

void store_destroy(store_t *st);

#endif
