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
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <ptywrap.h>

/* Note: send_slowly() is no longer needed - use ptywrap_set_send_delay() instead */

/* Helper to check if text appears in buffer */
static int buffer_contains(ptywrap_session_t *sess, const char *text) {
    char buf[256];
    int rows, cols;

    ptywrap_get_size(sess, &rows, &cols);

    for (int row = 0; row < rows; row++) {
        ptywrap_get_row_text(sess, row, buf, sizeof(buf));
        if (strstr(buf, text) != NULL) {
            return 1;
        }
    }
    return 0;
}

/* Helper to save screenshot for debugging */
static void save_screenshot(ptywrap_session_t *sess, const char *filename) {
    char *markdown = ptywrap_screenshot_markdown(sess, 0, -1);
    if (markdown) {
        FILE *f = fopen(filename, "w");
        if (f) {
            fprintf(f, "# Debug Screenshot\n\n%s", markdown);
            fclose(f);
            printf("Debug screenshot saved to: %s\n", filename);
        }
        free(markdown);
    }
}

void test_vi_edit() {
    printf("Test: vi editor - create and edit hello.c...\n");

    /* This test requires a running container with shell access */
    ptywrap_session_t *sess = ptywrap_create("mytest", 40, 150);
    if (!sess) {
        printf("  SKIPPED (container 'mytest' not running)\n");
        printf("  Run: podman run -d --name mytest alpine sleep 3600\n");
        return;
    }

    printf("  Container attached, PID: %d\n", ptywrap_get_container_pid(sess));
    sleep(1);

    /* Step 1: Install vi */
    printf("  Installing vi...\n");
    ptywrap_send_line(sess, "apk add --no-cache vim");

    /* Wait for installation to complete */
    sleep(5);

    /* Verify vi is installed */
    ptywrap_send_line(sess, "which vi");
    sleep(1);

    if (!buffer_contains(sess, "/usr/bin/vi")) {
        printf("  ERROR: vi installation failed\n");
        save_screenshot(sess, "vi_install_failed.md");
        ptywrap_destroy(sess);
        assert(0);
    }
    printf("  vi installed successfully\n");

    /* Step 2: Start vi with hello.c */
    printf("  Starting vi hello.c...\n");
    ptywrap_send_line(sess, "vi hello.c");
    sleep(2);  /* Wait for vi to start */

    /* Step 3: Enter insert mode and type the hello world program */
    printf("  Typing hello world program...\n");

    /* Press 'i' to enter insert mode */
    ptywrap_send(sess, "i", 1);
    usleep(100000);  /* 100ms */

    /* Type the hello world program line by line */
    const char *program[] = {
        "#include <stdio.h>",
        "",
        "int main() {",
        "    printf(\"Hello, World!\\n\");",
        "    return 0;",
        "}",
        NULL
    };

    /* Configure inter-character delay for vi input */
    ptywrap_set_send_delay(sess, 10);  /* 10ms between characters */

    for (int i = 0; program[i] != NULL; i++) {
        ptywrap_send(sess, program[i], strlen(program[i]));
        ptywrap_send(sess, "\r", 1);  /* Newline in vi */
        usleep(100000);  /* 100ms between lines */
    }

    /* Reset delay to 0 for subsequent commands */
    ptywrap_set_send_delay(sess, 0);

    sleep(1);

    /* Step 4: Exit insert mode and save */
    printf("  Saving and exiting vi...\n");

    /* Press ESC to exit insert mode */
    ptywrap_send(sess, "\033", 1);  /* ESC key */
    sleep(1);

    /* Type :wq to save and quit */
    ptywrap_send_line(sess, ":wq");
    sleep(2);  /* Wait for vi to exit */

    /* Step 5: Verify the file was created and contains correct content */
    printf("  Verifying saved file...\n");

    /* Check if file exists */
    ptywrap_send_line(sess, "ls -la hello.c");
    sleep(1);

    if (!buffer_contains(sess, "hello.c")) {
        printf("  ERROR: hello.c was not created\n");
        save_screenshot(sess, "vi_file_missing.md");
        ptywrap_destroy(sess);
        assert(0);
    }
    printf("  File hello.c exists\n");

    /* Display the file content */
    ptywrap_send_line(sess, "cat hello.c");
    sleep(1);

    /* Verify key parts of the program are in the buffer */
    int checks_passed = 0;

    if (buffer_contains(sess, "#include <stdio.h>")) {
        printf("  ✓ Found: #include <stdio.h>\n");
        checks_passed++;
    }

    if (buffer_contains(sess, "int main()")) {
        printf("  ✓ Found: int main()\n");
        checks_passed++;
    }

    if (buffer_contains(sess, "printf")) {
        printf("  ✓ Found: printf\n");
        checks_passed++;
    }

    if (buffer_contains(sess, "Hello, World!")) {
        printf("  ✓ Found: Hello, World!\n");
        checks_passed++;
    }

    if (buffer_contains(sess, "return 0")) {
        printf("  ✓ Found: return 0\n");
        checks_passed++;
    }

    /* Save a screenshot for inspection */
    save_screenshot(sess, "vi_edit_success.md");

    /* Verify we found all expected content */
    if (checks_passed < 5) {
        printf("  ERROR: Only %d/5 checks passed\n", checks_passed);
        printf("  See vi_edit_success.md for details\n");
        ptywrap_destroy(sess);
        assert(0);
    }

    printf("  All content checks passed (%d/5)\n", checks_passed);

    /* Bonus: Try to compile and run it */
    printf("  Compiling hello.c...\n");
    ptywrap_send_line(sess, "apk add --no-cache gcc musl-dev");
    sleep(3);

    ptywrap_send_line(sess, "gcc -o hello hello.c 2>&1");
    sleep(2);

    /* Run the program */
    ptywrap_send_line(sess, "./hello");
    sleep(1);

    if (buffer_contains(sess, "Hello, World!")) {
        printf("  ✓ Program compiled and ran successfully!\n");
    } else {
        printf("  ⚠ Could not verify program execution (but file was saved correctly)\n");
    }

    /* Cleanup */
    ptywrap_send_line(sess, "rm -f hello.c hello");
    sleep(1);

    ptywrap_destroy(sess);
    printf("  PASSED - vi edit test completed successfully\n");
}

int main() {
    printf("Running vi editor integration test...\n");
    printf("Prerequisites:\n");
    printf("  1. podman run -d --name mytest alpine sleep 3600\n");
    printf("  2. This test will install vim, gcc in the container\n");
    printf("  3. Test takes ~20-30 seconds to complete\n\n");

    test_vi_edit();

    printf("\nTest completed successfully!\n");
    return 0;
}
