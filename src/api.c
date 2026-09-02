/*
 * Framework for testing TTY based linux applications
 * Copyright (C) 2026 SUSE Linux 
 * 
 * Everyone is permitted to copy and distribute verbatim copies
 * of this license document, but changing it is not allowed.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * For the full text of the GNU General Public License version 2, see:
 * https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "../include/ptywrap.h"
#include "internal.h"
#include "pty.h"
#include "container.h"
#include "terminal.h"
#include "reader.h"
#include "screenshot.h"

ptywrap_session_t* ptywrap_create(const char *container_id, int rows, int cols) {
    if (!container_id) {
        errno = EINVAL;
        return NULL;
    }

    /* Use defaults if not specified */
    if (rows <= 0) rows = 40;
    if (cols <= 0) cols = 150;

    /* Allocate session */
    ptywrap_session_t *sess = calloc(1, sizeof(ptywrap_session_t));
    if (!sess) {
        return NULL;
    }

    sess->master_fd = -1;
    sess->rows = rows;
    sess->cols = cols;
    sess->exec_pid = -1;
    sess->send_delay_ms = 0;  /* No delay by default */

    /* Initialize mutex */
    if (pthread_mutex_init(&sess->buffer_lock, NULL) != 0) {
        free(sess);
        return NULL;
    }

    /* Store container ID */
    sess->container_id = strdup(container_id);
    if (!sess->container_id) {
        pthread_mutex_destroy(&sess->buffer_lock);
        free(sess);
        return NULL;
    }

    /* Create PTY */
    int ret = pty_create(sess);
    if (ret != PTYWRAP_OK) {
        free(sess->container_id);
        pthread_mutex_destroy(&sess->buffer_lock);
        free(sess);
        return NULL;
    }

    /* Initialize terminal buffer */
    ret = terminal_init(sess);
    if (ret != PTYWRAP_OK) {
        pty_close(sess);
        free(sess->container_id);
        pthread_mutex_destroy(&sess->buffer_lock);
        free(sess);
        return NULL;
    }

    /* Attach to existing container via podman exec */
    ret = container_attach(sess, container_id);
    if (ret != PTYWRAP_OK) {
        terminal_free(sess);
        pty_close(sess);
        free(sess->container_id);
        pthread_mutex_destroy(&sess->buffer_lock);
        free(sess);
        return NULL;
    }

    /* Start reader thread */
    ret = reader_start(sess);
    if (ret != PTYWRAP_OK) {
        terminal_free(sess);
        pty_close(sess);
        free(sess->container_id);
        pthread_mutex_destroy(&sess->buffer_lock);
        free(sess);
        return NULL;
    }

    return sess;
}

void ptywrap_destroy(ptywrap_session_t *session) {
    if (!session) {
        return;
    }

    /* Stop reader thread */
    reader_stop(session);

    /* Close PTY */
    pty_close(session);

    /* Free terminal buffer */
    terminal_free(session);

    /* Free container ID */
    if (session->container_id) {
        free(session->container_id);
    }

    /* Destroy mutex */
    pthread_mutex_destroy(&session->buffer_lock);

    /* Free session */
    free(session);
}

int ptywrap_send(ptywrap_session_t *session, const char *data, size_t len) {
    if (!session || !data || session->master_fd < 0) {
        return PTYWRAP_ERR_INVAL;
    }

    ssize_t total_written = 0;

    /* If inter-character delay is set, send character by character */
    if (session->send_delay_ms > 0) {
        for (size_t i = 0; i < len; i++) {
            ssize_t written = write(session->master_fd, &data[i], 1);
            if (written < 0) {
                return PTYWRAP_ERR_PTY;
            }
            total_written += written;

            /* Insert delay after each character (except the last) */
            if (i < len - 1) {
                usleep(session->send_delay_ms * 1000);
            }
        }
    } else {
        /* No delay - send all at once */
        ssize_t written = write(session->master_fd, data, len);
        if (written < 0) {
            return PTYWRAP_ERR_PTY;
        }
        total_written = written;
    }

    return (int)total_written;
}

int ptywrap_send_str(ptywrap_session_t *session, const char *str) {
    if (!str) {
        return PTYWRAP_ERR_INVAL;
    }
    return ptywrap_send(session, str, strlen(str));
}

int ptywrap_send_line(ptywrap_session_t *session, const char *str) {
    if (!str) {
        return PTYWRAP_ERR_INVAL;
    }

    int ret = ptywrap_send_str(session, str);
    if (ret < 0) {
        return ret;
    }

    int ret2 = ptywrap_send(session, "\n", 1);
    if (ret2 < 0) {
        return ret2;
    }

    return ret + ret2;
}

int ptywrap_get_size(ptywrap_session_t *session, int *rows, int *cols) {
    if (!session || !rows || !cols) {
        return PTYWRAP_ERR_INVAL;
    }

    *rows = session->rows;
    *cols = session->cols;

    return PTYWRAP_OK;
}

int ptywrap_get_cell(ptywrap_session_t *session, int row, int col,
                      ptywrap_cell_t *cell) {
    if (!session || !cell) {
        return PTYWRAP_ERR_INVAL;
    }

    if (row < 0 || row >= session->rows || col < 0 || col >= session->cols) {
        return PTYWRAP_ERR_INVAL;
    }

    pthread_mutex_lock(&session->buffer_lock);
    int idx = row * session->cols + col;
    *cell = session->buffer[idx];
    pthread_mutex_unlock(&session->buffer_lock);

    return PTYWRAP_OK;
}

int ptywrap_get_row(ptywrap_session_t *session, int row,
                     ptywrap_cell_t *cells) {
    if (!session || !cells) {
        return PTYWRAP_ERR_INVAL;
    }

    if (row < 0 || row >= session->rows) {
        return PTYWRAP_ERR_INVAL;
    }

    pthread_mutex_lock(&session->buffer_lock);
    int start_idx = row * session->cols;
    memcpy(cells, &session->buffer[start_idx],
           session->cols * sizeof(ptywrap_cell_t));
    pthread_mutex_unlock(&session->buffer_lock);

    return PTYWRAP_OK;
}

int ptywrap_get_cursor(ptywrap_session_t *session, int *row, int *col) {
    if (!session || !row || !col) {
        return PTYWRAP_ERR_INVAL;
    }

    pthread_mutex_lock(&session->buffer_lock);
    *row = session->cursor_row;
    *col = session->cursor_col;
    pthread_mutex_unlock(&session->buffer_lock);

    return PTYWRAP_OK;
}

int ptywrap_get_row_text(ptywrap_session_t *session, int row,
                          char *buf, size_t buflen) {
    if (!session || !buf || buflen == 0) {
        return PTYWRAP_ERR_INVAL;
    }

    if (row < 0 || row >= session->rows) {
        return PTYWRAP_ERR_INVAL;
    }

    pthread_mutex_lock(&session->buffer_lock);

    int start_idx = row * session->cols;
    size_t chars_to_copy = session->cols;
    if (chars_to_copy >= buflen) {
        chars_to_copy = buflen - 1;
    }

    for (size_t i = 0; i < chars_to_copy; i++) {
        buf[i] = session->buffer[start_idx + i].ch;
    }
    buf[chars_to_copy] = '\0';

    pthread_mutex_unlock(&session->buffer_lock);

    return (int)chars_to_copy;
}

int ptywrap_container_alive(ptywrap_session_t *session) {
    if (!session) {
        return PTYWRAP_ERR_INVAL;
    }

    return container_exec_alive(session);
}

pid_t ptywrap_get_container_pid(ptywrap_session_t *session) {
    if (!session) {
        return PTYWRAP_ERR_INVAL;
    }

    return session->exec_pid;
}

char* ptywrap_screenshot_markdown(ptywrap_session_t *session,
                                   int start_row, int end_row) {
    if (!session) {
        return NULL;
    }

    /* Handle -1 for end_row (means last row) */
    if (end_row < 0) {
        end_row = session->rows - 1;
    }

    return screenshot_to_markdown(session, start_row, end_row);
}

int ptywrap_set_send_delay(ptywrap_session_t *session, int delay_ms) {
    if (!session) {
        return PTYWRAP_ERR_INVAL;
    }

    /* Negative values are treated as 0 */
    session->send_delay_ms = (delay_ms < 0) ? 0 : delay_ms;

    return PTYWRAP_OK;
}

int ptywrap_get_send_delay(ptywrap_session_t *session) {
    if (!session) {
        return PTYWRAP_ERR_INVAL;
    }

    return session->send_delay_ms;
}
