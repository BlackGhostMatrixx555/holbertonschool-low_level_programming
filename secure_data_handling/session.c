#include <stdlib.h>
#include <string.h>
#include "session.h"

/**
 * session_create - Allocates and initialises a new session
 * @id: session identifier string (will be duplicated)
 * @uid: user id
 * @data: raw data buffer (will be copied)
 * @data_len: length of data in bytes
 *
 * Return: pointer to the new session, or NULL on failure
 *
 * Bugs fixed:
 *   - id was stored as a raw pointer into the caller's buffer (stack / input
 *     line).  session_destroy() calls free(s->id), which is undefined
 *     behaviour when the pointer was not heap-allocated.  Fix: strdup(id).
 *   - On malloc failure for the data buffer, 's' itself was leaked because
 *     the early return did not free it.  Fix: free(s) before returning NULL.
 */
session_t *session_create(const char *id, unsigned int uid,
			const unsigned char *data, size_t data_len)
{
	session_t *s;

	s = (session_t *)malloc(sizeof(*s));
	if (!s)
		return (NULL);

	/* BUG FIX 1: duplicate id so session owns its own heap copy */
	s->id = strdup(id);
	if (!s->id)
	{
		free(s);
		return (NULL);
	}

	s->uid = uid;

	if (data_len > 0)
	{
		s->data = (unsigned char *)malloc(data_len);
		if (!s->data)
		{
			/* BUG FIX 2: free s (and the id we just duped) before returning */
			free(s->id);
			free(s);
			return (NULL);
		}
		memcpy(s->data, data, data_len);
		s->data_len = data_len;
	}
	else
	{
		s->data = NULL;
		s->data_len = 0;
	}

	return (s);
}

/**
 * session_set_data - Replaces the data buffer of a session
 * @s: target session
 * @data: new data buffer
 * @data_len: length in bytes (0 to clear)
 *
 * Return: 1 on success, 0 on failure
 *
 * Bug fixed:
 *   - realloc() result was assigned directly to s->data.  If realloc()
 *     returns NULL the original pointer is lost (memory leak) and s->data
 *     becomes NULL, corrupting the session state.
 *     Fix: keep the result in a temporary variable and update s->data only
 *     on success.
 */
int session_set_data(session_t *s, const unsigned char *data, size_t data_len)
{
	unsigned char *tmp;

	if (!s)
		return (0);

	if (data_len == 0)
	{
		free(s->data);
		s->data = NULL;
		s->data_len = 0;
		return (1);
	}

	/* BUG FIX 3: use tmp so the original s->data is not lost on failure */
	tmp = (unsigned char *)realloc(s->data, data_len);
	if (!tmp)
		return (0);

	s->data = tmp;
	memcpy(s->data, data, data_len);
	s->data_len = data_len;
	return (1);
}

/**
 * session_destroy - Frees all memory owned by a session
 * @s: session to destroy (may be NULL)
 */
void session_destroy(session_t *s)
{
	if (!s)
		return;

	free(s->id);
	free(s->data);
	free(s);
}
