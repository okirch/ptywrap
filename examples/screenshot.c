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
#include <unistd.h>
#include <ptywrap.h>

int main() {
    printf("PTY Screenshot Example\n");
    printf("======================\n\n");

    printf("Prerequisites:\n");
    printf("  podman run -d --name mytest alpine sleep 3600\n\n");

    /* Attach to container */
    ptywrap_session_t *sess = ptywrap_create("mytest", 0, 0);
    if (!sess) {
        perror("Failed to attach to container 'mytest'");
        printf("Make sure container is running!\n");
        return 1;
    }

    printf("Attached to container. Sending commands...\n\n");

    sleep(1);

    /* Send some commands to generate interesting output */
    ptywrap_send_line(sess, "echo 'Hello from PTY wrapper'");
    sleep(1);

    ptywrap_send_line(sess, "ls -la /");
    sleep(1);

    ptywrap_send_line(sess, "pwd");
    sleep(1);

    /* Take a screenshot of the first 20 rows */
    printf("Taking markdown screenshot (rows 0-19)...\n");
    char *markdown = ptywrap_screenshot_markdown(sess, 0, 19);

    if (markdown) {
        printf("\n--- Markdown Output ---\n");
        printf("%s", markdown);
        printf("--- End Output ---\n\n");

        /* Optionally save to file */
        FILE *f = fopen("screenshot.md", "w");
        if (f) {
            fprintf(f, "# Terminal Screenshot\n\n");
            fprintf(f, "%s", markdown);
            fclose(f);
            printf("Screenshot saved to: screenshot.md\n");
        }

        free(markdown);
    } else {
        printf("Failed to generate screenshot!\n");
    }

    /* Take another screenshot of just a few rows */
    printf("\nTaking screenshot of rows 0-5 only...\n");
    markdown = ptywrap_screenshot_markdown(sess, 0, 5);
    if (markdown) {
        printf("\n%s", markdown);
        free(markdown);
    }

    /* Cleanup */
    ptywrap_destroy(sess);

    printf("\nDone. Container 'mytest' is still running.\n");
    return 0;
}
