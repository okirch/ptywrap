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
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "container.h"
#include "internal.h"

int container_attach(ptywrap_session_t *sess, const char *container_id) {
    if (!sess || !container_id || !sess->slave_name) {
        return PTYWRAP_ERR_INVAL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return PTYWRAP_ERR_CONTAINER;
    }

    if (pid == 0) {
        /* Child process */

        /* Close master fd (inherited from parent) */
        close(sess->master_fd);

        /* Open slave PTY and set it as stdin/stdout/stderr */
        int slave_fd = open(sess->slave_name, O_RDWR);
        if (slave_fd < 0) {
            _exit(126);
        }

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        if (slave_fd > 2) {
            close(slave_fd);
        }

        /* Execute podman exec to attach to existing container */
        execlp("podman", "podman", "exec",
               "-i",
               "-t",
               container_id,
               "/bin/sh",
               NULL);

        /* exec failed */
        _exit(127);
    }

    /* Parent process */
    sess->exec_pid = pid;

    /* Give exec a moment to start */
    usleep(100000); /* 100ms */

    return PTYWRAP_OK;
}

int container_exec_alive(ptywrap_session_t *sess) {
    return process_pid_alive(sess);
}

int direct_spawn(ptywrap_session_t *sess, char *const argv[]) {
    if (!sess || !argv || !argv[0] || !sess->slave_name) {
        return PTYWRAP_ERR_INVAL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return PTYWRAP_ERR_CONTAINER;
    }

    if (pid == 0) {
        /* Child process */

        /* Close master fd (inherited from parent) */
        close(sess->master_fd);

        /* Open slave PTY and set it as stdin/stdout/stderr */
        int slave_fd = open(sess->slave_name, O_RDWR);
        if (slave_fd < 0) {
            _exit(126);
        }

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        if (slave_fd > 2) {
            close(slave_fd);
        }

        /* Execute command directly, searching PATH */
        execvp(argv[0], argv);

        /* exec failed */
        _exit(127);
    }

    /* Parent process */
    sess->exec_pid = pid;

    /* Give process a moment to start */
    usleep(100000); /* 100ms */

    return PTYWRAP_OK;
}

int process_pid_alive(ptywrap_session_t *sess) {
    if (!sess || sess->exec_pid <= 0) {
        return PTYWRAP_ERR_INVAL;
    }

    int status;
    pid_t result = waitpid(sess->exec_pid, &status, WNOHANG);

    if (result == 0) {
        /* Still running */
        return 1;
    } else if (result == sess->exec_pid) {
        /* Exited */
        return 0;
    } else {
        /* Error */
        return PTYWRAP_ERR_CONTAINER;
    }
}
