#ifndef SESSION_H
#define SESSION_H

#include <stddef.h>

typedef struct session_s {
	char *id;          /* expected to be a string */
	unsigned int uid;  /* user id */
	unsigned char *data;
	size_t data_len;
} session_t;

session_t *session_create(const char *id, unsigned int uid, const unsigned char *data, size_t data_len);
void session_destroy(session_t *s);

/* Update the data buffer */
int session_set_data(session_t *s, const unsigned char *data, size_t data_len);

#endif
