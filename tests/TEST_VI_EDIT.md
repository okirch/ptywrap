# Vi Editor Integration Test

## Overview

`test_vi_edit.c` is a comprehensive integration test that demonstrates ptywrap's ability to interact with complex terminal applications like vi/vim.

## Test Scenario

The test performs the following steps:

### 1. Install Vi
- Connects to running Alpine container
- Runs `apk add --no-cache vim`
- Verifies installation with `which vi`

### 2. Start Vi Editor
- Launches `vi hello.c`
- Waits for editor to fully load

### 3. Create Hello World Program
- Enters insert mode (`i` key)
- Types a complete C hello world program:
  ```c
  #include <stdio.h>
  
  int main() {
      printf("Hello, World!\n");
      return 0;
  }
  ```
- Exits insert mode (ESC key)

### 4. Save and Exit
- Sends `:wq` command
- Waits for vi to exit

### 5. Verify File Content
- Lists the file with `ls -la hello.c`
- Displays content with `cat hello.c`
- Checks buffer contains all expected code:
  - `#include <stdio.h>`
  - `int main()`
  - `printf`
  - `Hello, World!`
  - `return 0`

### 6. Compile and Run (Bonus)
- Installs gcc compiler
- Compiles the program: `gcc -o hello hello.c`
- Runs the executable: `./hello`
- Verifies output contains "Hello, World!"

### 7. Cleanup
- Removes created files
- Closes session

## Key Features Demonstrated

### Character-by-Character Input
```c
send_slowly(sess, program[i], 10);  /* 10ms between chars */
```

### Special Key Handling
```c
ptywrap_send(sess, "\033", 1);  /* ESC key */
ptywrap_send(sess, "\r", 1);    /* Return/Enter */
```

### Buffer Content Search
```c
if (buffer_contains(sess, "#include <stdio.h>")) {
    printf("✓ Found: #include <stdio.h>\n");
}
```

### Debug Screenshots
```c
save_screenshot(sess, "vi_edit_success.md");
```

## Helper Functions

### send_slowly()
Sends string character-by-character with configurable delay between characters. Essential for reliable vi interaction.

### buffer_contains()
Searches entire terminal buffer for a text string. Used to verify file content.

### save_screenshot()
Saves markdown screenshot for debugging when tests fail or succeed.

## Running the Test

### Prerequisites
```bash
# Start Alpine container
podman run -d --name mytest alpine sleep 3600
```

### Build and Run
```bash
# Compile
gcc -o test_vi_edit test_vi_edit.c -I../include -L.. -lptywrap -pthread

# Run
LD_LIBRARY_PATH=.. ./test_vi_edit
```

### Expected Output
```
Running vi editor integration test...
Prerequisites:
  1. podman run -d --name mytest alpine sleep 3600
  2. This test will install vim, gcc in the container
  3. Test takes ~20-30 seconds to complete

Test: vi editor - create and edit hello.c...
  Container attached, PID: 23165
  Installing vi...
  vi installed successfully
  Starting vi hello.c...
  Typing hello world program...
  Saving and exiting vi...
  Verifying saved file...
  File hello.c exists
  ✓ Found: #include <stdio.h>
  ✓ Found: int main()
  ✓ Found: printf
  ✓ Found: Hello, World!
  ✓ Found: return 0
Debug screenshot saved to: vi_edit_success.md
  All content checks passed (5/5)
  Compiling hello.c...
  ✓ Program compiled and ran successfully!
  PASSED - vi edit test completed successfully

Test completed successfully!
```

## Timing Considerations

The test includes strategic delays:
- **1s** after container attach (shell startup)
- **5s** after vi installation (package download/install)
- **2s** after starting vi (editor initialization)
- **100ms** between keystrokes (reliable input)
- **100ms** between lines (vi processing)
- **1s** after ESC (mode transition)
- **2s** after :wq (vi exit and file save)

These delays ensure reliable operation across different system loads.

## Debugging Failed Tests

If the test fails, check the debug screenshots:

- `vi_install_failed.md` - Vi installation didn't complete
- `vi_file_missing.md` - File wasn't created
- `vi_edit_success.md` - Shows final terminal state (always saved)

## What This Test Proves

1. **PTY Control Characters Work**: ESC, Enter, special sequences
2. **Terminal Emulation Accuracy**: Vi requires proper VT100 emulation
3. **Timing Handling**: Proper delays for interactive applications
4. **Buffer Accuracy**: Terminal buffer correctly captures all output
5. **Real-World Applicability**: Can automate complex terminal workflows

## Practical Applications

This test demonstrates ptywrap's capability for:
- Automated testing of terminal-based editors (vi, vim, nano, emacs)
- Interactive configuration tools
- Terminal-based installers
- Any application requiring complex keyboard input
- Test automation for legacy TUI applications

## Technical Notes

### Why Character-by-Character?

Sending text slowly (`send_slowly()`) is more reliable than bulk sends because:
1. Vi processes each character individually
2. Input buffer overflow can cause missed characters
3. Terminal processing needs time between characters
4. More closely mimics actual user typing

### ESC Key Handling

The ESC key (`\033`) is critical for vi:
- Exits insert mode
- Cancels partial commands
- Returns to command mode

Proper ESC handling proves ptywrap correctly handles control characters.

## Verification

The test verifies correctness at multiple levels:

1. **Process level**: Vi executable exists
2. **Application level**: Vi starts and responds
3. **File system level**: File is created
4. **Content level**: File contains correct code
5. **Compilation level**: Code compiles without errors
6. **Execution level**: Program runs and produces correct output

This multi-level verification ensures the entire workflow works correctly.
