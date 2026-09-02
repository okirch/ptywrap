#ifndef PTY_H
#define PTY_H

#include "internal.h"

/* Create PTY master/slave pair
 * Returns: PTYWRAP_OK on success, error code on failure
 */
int pty_create(ptywrap_session_t *sess);

/* Close PTY master */
void pty_close(ptywrap_session_t *sess);

#endif /* PTY_H */
