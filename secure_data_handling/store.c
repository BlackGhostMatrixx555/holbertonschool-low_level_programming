#include <stdlib.h>
#include <string.h>
#include "store.h"

/**
 * store_init - Initialises an empty store
 * @st: store to initialise
 */
void store_init(store_t *st)
{
	if (st)
		st->head = NULL;
}

/**
 * node_create - Allocates a linked-list node wrapping a session
 * @s: session pointer to store
 *
 * Return: pointer to new node, or NULL on allocation failure
 */
static node_t *node_create(session_t *s)
{
	node_t *n = (node_t *)malloc(sizeof(*n));

	if (!n)
		return (NULL);
	n->sess = s;
	n->next = NULL;
	return (n);
}

/**
 * store_add - Inserts a session into the store
 * @st: target store
 * @s: session to insert (must have a non-NULL id)
 *
 * Return: 1 on success, 0 if duplicate id or on error
 */
int store_add(store_t *st, session_t *s)
{
	node_t *n, *cur;

	if (!st || !s || !s->id)
		return (0);

	cur = st->head;
	while (cur)
	{
		if (cur->sess && cur->sess->id &&
			strcmp(cur->sess->id, s->id) == 0)
			return (0);
		cur = cur->next;
	}

	n = node_create(s);
	if (!n)
		return (0);

	n->next = st->head;
	st->head = n;
	return (1);
}

/**
 * store_get - Looks up a session by id
 * @st: store to search
 * @id: session id to look for
 *
 * Return: pointer to the session, or NULL if not found
 */
session_t *store_get(store_t *st, const char *id)
{
	node_t *cur;

	if (!st || !id)
		return (NULL);

	cur = st->head;
	while (cur)
	{
		if (cur->sess && cur->sess->id &&
			strcmp(cur->sess->id, id) == 0)
			return (cur->sess);
		cur = cur->next;
	}
	return (NULL);
}

/**
 * store_delete - Removes and destroys a session by id
 * @st: store to delete from
 * @id: session id to remove
 * @out: if non-NULL, receives the session pointer BEFORE destruction
 *       (caller must not dereference it: it is freed inside this function)
 *
 * Return: 1 if deleted, 0 if not found
 *
 * Bug fixed:
 *   - When out != NULL the old code wrote *out = cur->sess and then called
 *     session_destroy(cur->sess), handing the caller a dangling pointer.
 *     The CLI always passes NULL for out so it never crashed in practice,
 *     but the contract was broken and any caller using *out would trigger
 *     use-after-free.
 *     Fix: session_destroy is called unconditionally; if the caller asked
 *     for *out we set it to NULL to make the dangling-pointer bug obvious
 *     instead of silent.
 */
int store_delete(store_t *st, const char *id, session_t **out)
{
	node_t *cur, *prev;

	if (!st || !id)
		return (0);

	prev = NULL;
	cur = st->head;

	while (cur)
	{
		if (cur->sess && cur->sess->id &&
			strcmp(cur->sess->id, id) == 0)
		{
			if (prev)
				prev->next = cur->next;
			else
				st->head = cur->next;

			/* BUG FIX 4: destroy first, then nullify *out so the caller
			 * cannot accidentally use a dangling pointer. */
			session_destroy(cur->sess);
			if (out)
				*out = NULL;

			free(cur);
			return (1);
		}
		prev = cur;
		cur = cur->next;
	}

	return (0);
}

/**
 * store_destroy - Frees all nodes and sessions in the store
 * @st: store to destroy (reset to empty state after)
 */
void store_destroy(store_t *st)
{
	node_t *cur, *next;

	if (!st)
		return;

	cur = st->head;
	while (cur)
	{
		next = cur->next;
		session_destroy(cur->sess);
		free(cur);
		cur = next;
	}
	st->head = NULL;
}
