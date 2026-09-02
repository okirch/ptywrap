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
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ptywrap.h>

void test_create_destroy() {
    printf("Test: attach and destroy session...\n");

    /* Assumes container 'mytest' is running */
    ptywrap_session_t *sess = ptywrap_create("mytest", 40, 150);
    if (!sess) {
        printf("  SKIPPED (container 'mytest' not running)\n");
        printf("  Run: podman run -d --name mytest alpine sleep 3600\n");
        return;
    }

    int rows, cols;
    int ret = ptywrap_get_size(sess, &rows, &cols);
    assert(ret == PTYWRAP_OK);
    assert(rows == 40);
    assert(cols == 150);

    ptywrap_destroy(sess);
    printf("  PASSED\n");
}

void test_send_receive() {
    printf("Test: send command and check buffer...\n");

    ptywrap_session_t *sess = ptywrap_create("mytest", 40, 150);
    if (!sess) {
        printf("  SKIPPED (container 'mytest' not running)\n");
        return;
    }

    sleep(1);  /* Wait for exec */

    /* Send echo command */
    int ret = ptywrap_send_line(sess, "echo TESTSTRING");
    assert(ret > 0);

    sleep(1);  /* Wait for output */

    /* Search buffer for our test string */
    char buf[256];
    int found = 0;
    for (int row = 0; row < 40; row++) {
        ptywrap_get_row_text(sess, row, buf, sizeof(buf));
        if (strstr(buf, "TESTSTRING")) {
            found = 1;
            break;
        }
    }

    assert(found);

    ptywrap_destroy(sess);
    printf("  PASSED\n");
}

void test_cursor() {
    printf("Test: cursor position tracking...\n");

    ptywrap_session_t *sess = ptywrap_create("mytest", 40, 150);
    if (!sess) {
        printf("  SKIPPED (container 'mytest' not running)\n");
        return;
    }

    int row, col;
    int ret = ptywrap_get_cursor(sess, &row, &col);
    assert(ret == PTYWRAP_OK);

    /* Initial cursor should be at 0,0 or near top */
    assert(row >= 0 && row < 40);
    assert(col >= 0 && col < 150);

    ptywrap_destroy(sess);
    printf("  PASSED\n");
}

void test_exec_status() {
    printf("Test: exec status check...\n");

    ptywrap_session_t *sess = ptywrap_create("mytest", 40, 150);
    if (!sess) {
        printf("  SKIPPED (container 'mytest' not running)\n");
        return;
    }

    sleep(1);

    pid_t pid = ptywrap_get_container_pid(sess);
    assert(pid > 0);

    int alive = ptywrap_container_alive(sess);
    assert(alive == 1);

    /* Send exit to shell (exits exec, not container) */
    ptywrap_send_line(sess, "exit");
    sleep(2);

    /* Exec should have exited (but container still running) */
    alive = ptywrap_container_alive(sess);
    assert(alive == 0);

    ptywrap_destroy(sess);
    printf("  PASSED\n");
}

int main() {
    printf("Running ptywrap basic tests...\n");
    printf("Prerequisites: podman run -d --name mytest alpine sleep 3600\n\n");

    test_create_destroy();
    test_send_receive();
    test_cursor();
    test_exec_status();

    printf("\nAll tests completed!\n");
    return 0;
}
