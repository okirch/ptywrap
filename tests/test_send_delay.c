/*
 * Framework for testing TTY based linux applications
 * Copyright (C) 2026 SUSE Linux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <ptywrap.h>

void test_send_delay() {
    printf("Test: send delay configuration...\n");

    /* This test requires a running container */
    ptywrap_session_t *sess = ptywrap_create("mytest", 40, 150);
    if (!sess) {
        printf("  SKIPPED (container 'mytest' not running)\n");
        printf("  Run: podman run -d --name mytest alpine sleep 3600\n");
        return;
    }

    printf("  Container attached\n");

    /* Test 1: Default delay should be 0 */
    int delay = ptywrap_get_send_delay(sess);
    assert(delay == 0);
    printf("  ✓ Default delay is 0ms\n");

    /* Test 2: Set positive delay */
    int ret = ptywrap_set_send_delay(sess, 50);
    assert(ret == PTYWRAP_OK);
    delay = ptywrap_get_send_delay(sess);
    assert(delay == 50);
    printf("  ✓ Set and get delay: 50ms\n");

    /* Test 3: Negative delay gets clamped to 0 */
    ret = ptywrap_set_send_delay(sess, -100);
    assert(ret == PTYWRAP_OK);
    delay = ptywrap_get_send_delay(sess);
    assert(delay == 0);
    printf("  ✓ Negative delay clamped to 0\n");

    /* Test 4: Verify delay affects timing (send 10 chars with 10ms delay) */
    ptywrap_set_send_delay(sess, 10);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Send 10 characters - should take at least 90ms (9 delays) */
    ptywrap_send(sess, "0123456789", 10);

    clock_gettime(CLOCK_MONOTONIC, &end);

    long elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 +
                     (end.tv_nsec - start.tv_nsec) / 1000000;

    printf("  ✓ 10 chars with 10ms delay took %ld ms (expected >= 90ms)\n", elapsed_ms);
    assert(elapsed_ms >= 80);  /* Allow some margin */

    /* Test 5: Verify no delay is fast */
    ptywrap_set_send_delay(sess, 0);

    clock_gettime(CLOCK_MONOTONIC, &start);
    ptywrap_send(sess, "abcdefghij", 10);
    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 +
                 (end.tv_nsec - start.tv_nsec) / 1000000;

    printf("  ✓ 10 chars with 0ms delay took %ld ms (expected < 10ms)\n", elapsed_ms);
    assert(elapsed_ms < 20);  /* Should be very fast */

    ptywrap_destroy(sess);
    printf("  PASSED - send delay API works correctly\n");
}

int main() {
    printf("Running send delay test...\n");
    printf("Prerequisites:\n");
    printf("  1. podman run -d --name mytest alpine sleep 3600\n\n");

    test_send_delay();

    printf("\nTest completed successfully!\n");
    return 0;
}
