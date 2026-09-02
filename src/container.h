#ifndef CONTAINER_H
#define CONTAINER_H

#include "internal.h"

/* Attach to existing container via podman exec
 * Returns: PTYWRAP_OK on success, error code on failure
 */
int container_attach(ptywrap_session_t *sess, const char *container_id);

/* Check if exec process is still running
 * Returns: 1 if alive, 0 if dead, negative on error
 */
int container_exec_alive(ptywrap_session_t *sess);

#endif /* CONTAINER_H */
