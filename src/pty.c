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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "pty.h"
#include "internal.h"

int pty_create(ptywrap_session_t *sess) {
    /* Open PTY master */
    sess->master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (sess->master_fd < 0) {
        return PTYWRAP_ERR_PTY;
    }

    /* Grant access and unlock */
    if (grantpt(sess->master_fd) < 0 || unlockpt(sess->master_fd) < 0) {
        close(sess->master_fd);
        sess->master_fd = -1;
        return PTYWRAP_ERR_PTY;
    }

    /* Get slave name */
    char *slave = ptsname(sess->master_fd);
    if (!slave) {
        close(sess->master_fd);
        sess->master_fd = -1;
        return PTYWRAP_ERR_PTY;
    }
    sess->slave_name = strdup(slave);
    if (!sess->slave_name) {
        close(sess->master_fd);
        sess->master_fd = -1;
        return PTYWRAP_ERR_NOMEM;
    }

    /* Set non-blocking mode */
    int flags = fcntl(sess->master_fd, F_GETFL);
    if (flags >= 0) {
        fcntl(sess->master_fd, F_SETFL, flags | O_NONBLOCK);
    }

    return PTYWRAP_OK;
}

void pty_close(ptywrap_session_t *sess) {
    if (sess->master_fd >= 0) {
        close(sess->master_fd);
        sess->master_fd = -1;
    }
    if (sess->slave_name) {
        free(sess->slave_name);
        sess->slave_name = NULL;
    }
}
