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
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ptywrap.h>

void test_direct_shell() {
    printf("Test: direct shell spawning...\n");

    /* Define arguments to run a local shell directly */
    char *const argv[] = { "/bin/sh", "-i", NULL };

    /* Create the direct PTY session */
    ptywrap_session_t *sess = ptywrap_create_direct(argv, 40, 150);
    assert(sess != NULL);
    printf("  Direct session created with PID: %d\n", ptywrap_get_process_pid(sess));

    sleep(1); /* Allow shell initialization */

    /* Ensure process is alive */
    assert(ptywrap_process_alive(sess) == 1);

    /* Send echo command */
    int ret = ptywrap_send_line(sess, "echo DIRECT_TEST_OK");
    assert(ret > 0);

    sleep(1); /* Allow output to settle */

    /* Verify the terminal buffer contains the expected string */
    char buf[256];
    int found = 0;
    for (int row = 0; row < 40; row++) {
        ptywrap_get_row_text(sess, row, buf, sizeof(buf));
        if (strstr(buf, "DIRECT_TEST_OK")) {
            found = 1;
            break;
        }
    }
    assert(found);
    printf("  ✓ Found 'DIRECT_TEST_OK' in terminal buffer\n");

    /* Terminate the direct shell session */
    ptywrap_send_line(sess, "exit");
    sleep(1);

    /* Verify process status reflects exit */
    assert(ptywrap_process_alive(sess) == 0);
    printf("  ✓ Process correctly detected as exited\n");

    ptywrap_destroy(sess);
    printf("  PASSED\n");
}

int main() {
    printf("Running ptywrap direct execution tests...\n");
    test_direct_shell();
    printf("\nAll direct tests completed successfully!\n");
    return 0;
}
