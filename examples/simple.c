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

#include <stdio.h>
#include <unistd.h>
#include <ptywrap.h>

int main() {
    printf("Attaching PTY session to running container...\n");
    printf("(Make sure you have a running container named 'mytest')\n");
    printf("Example: podman run -d --name mytest alpine sleep 3600\n\n");

    /* Attach to existing running container */
    ptywrap_session_t *sess = ptywrap_create("mytest", 0, 0);
    if (!sess) {
        perror("ptywrap_create failed - is container 'mytest' running?");
        return 1;
    }

    printf("Session attached. Exec PID: %d\n", ptywrap_get_container_pid(sess));

    /* Wait for exec to fully attach */
    sleep(1);

    /* Send a command */
    printf("Sending command: echo 'Hello from container'\n");
    ptywrap_send_line(sess, "echo 'Hello from container'");

    /* Wait for output */
    sleep(1);

    /* Read and display buffer content */
    printf("\nTerminal buffer content:\n");
    printf("========================\n");

    int rows, cols;
    ptywrap_get_size(sess, &rows, &cols);

    char buf[256];
    for (int row = 0; row < 10; row++) {  /* Show first 10 rows */
        int len = ptywrap_get_row_text(sess, row, buf, sizeof(buf));
        if (len > 0) {
            printf("Row %2d: %s\n", row, buf);
        }
    }

    printf("========================\n");

    /* Get cursor position */
    int cursor_row, cursor_col;
    ptywrap_get_cursor(sess, &cursor_row, &cursor_col);
    printf("Cursor position: row %d, col %d\n", cursor_row, cursor_col);

    /* Check if container is still running */
    int alive = ptywrap_container_alive(sess);
    printf("Container status: %s\n", alive > 0 ? "running" : "stopped");

    /* Send exit command */
    printf("\nSending exit command...\n");
    ptywrap_send_line(sess, "exit");
    sleep(1);

    /* Cleanup */
    printf("Destroying session...\n");
    ptywrap_destroy(sess);

    printf("Done. Container 'mytest' is still running.\n");
    printf("Use 'podman ps' to see it, 'podman stop mytest' to stop it.\n");

    return 0;
}
