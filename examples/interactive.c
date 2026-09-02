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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ptywrap.h>

void display_buffer(ptywrap_session_t *sess, int num_rows) {
    char buf[256];
    printf("\n--- Terminal Buffer (first %d rows) ---\n", num_rows);
    for (int row = 0; row < num_rows; row++) {
        ptywrap_get_row_text(sess, row, buf, sizeof(buf));
        printf("%s\n", buf);
    }
    printf("--- End Buffer ---\n\n");
}

int main(int argc, char *argv[]) {
    const char *container_id = "mytest";

    if (argc > 1) {
        container_id = argv[1];
    }

    printf("Attaching PTY session to container: %s\n", container_id);
    printf("(Make sure container is already running)\n");
    printf("Example: podman run -d --name %s alpine sleep 3600\n\n", container_id);

    ptywrap_session_t *sess = ptywrap_create(container_id, 40, 150);
    if (!sess) {
        perror("ptywrap_create failed - is the container running?");
        return 1;
    }

    printf("Attached to container (Exec PID: %d)\n", ptywrap_get_container_pid(sess));
    printf("Waiting for exec to attach...\n");
    sleep(1);

    printf("\nInteractive mode. Commands:\n");
    printf("  Type shell commands to send to container\n");
    printf("  'show' - display terminal buffer\n");
    printf("  'quit' - exit\n\n");

    char input[256];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        /* Remove trailing newline */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (strcmp(input, "quit") == 0) {
            break;
        } else if (strcmp(input, "show") == 0) {
            display_buffer(sess, 20);

            int row, col;
            ptywrap_get_cursor(sess, &row, &col);
            printf("Cursor: row %d, col %d\n", row, col);

            int alive = ptywrap_container_alive(sess);
            printf("Container: %s\n", alive > 0 ? "running" : "stopped");
        } else {
            /* Send command to container */
            ptywrap_send_line(sess, input);

            /* Give it time to execute */
            usleep(500000);  /* 500ms */
        }
    }

    printf("\nDestroying session...\n");
    ptywrap_destroy(sess);
    printf("Done.\n");

    return 0;
}
