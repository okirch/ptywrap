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

#include <unistd.h>
#include <errno.h>

#include "reader.h"
#include "terminal.h"
#include "internal.h"

void* reader_thread_func(void *arg) {
    ptywrap_session_t *sess = (ptywrap_session_t*)arg;
    char buf[4096];

    while (sess->reader_running) {
        ssize_t n = read(sess->master_fd, buf, sizeof(buf));

        if (n > 0) {
            pthread_mutex_lock(&sess->buffer_lock);
            terminal_process_bytes(sess, buf, n);
            pthread_mutex_unlock(&sess->buffer_lock);
        } else if (n == 0) {
            /* EOF - container exited */
            break;
        } else if (errno != EAGAIN && errno != EINTR) {
            /* Real error */
            break;
        }

        usleep(1000); /* 1ms poll interval */
    }

    return NULL;
}

int reader_start(ptywrap_session_t *sess) {
    if (!sess) {
        return PTYWRAP_ERR_INVAL;
    }

    sess->reader_running = 1;

    if (pthread_create(&sess->reader_thread, NULL, reader_thread_func, sess) != 0) {
        sess->reader_running = 0;
        return PTYWRAP_ERR_THREAD;
    }

    return PTYWRAP_OK;
}

void reader_stop(ptywrap_session_t *sess) {
    if (!sess) {
        return;
    }

    if (sess->reader_running) {
        sess->reader_running = 0;
        pthread_join(sess->reader_thread, NULL);
    }
}
