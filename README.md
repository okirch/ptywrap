# ptywrap - PTY Container Emulator Library

A C shared library for creating pseudo-terminal (PTY) sessions that attach to existing containerized applications via `podman exec`.

## Features

- **PTY Management**: Creates and manages PTY master/slave pairs using POSIX APIs
- **Container Integration**: Attaches to existing running containers via `podman exec -it`
- **VT100/ANSI Emulation**: Full terminal emulation with color and attribute support
- **Thread-Safe**: Synchronous API with mutex-protected buffer access
- **Non-Destructive**: Does not create or destroy containers, only attaches to existing ones

## Requirements

- GCC or Clang with C11 support
- pthread library
- podman (must be installed and accessible in PATH)
- Linux with POSIX PTY support

## Building

```bash
cd ptywrap
make
```

This creates the versioned shared library:
- `libptywrap.so.0.1` - Real library file (version 0.1)
- `libptywrap.so.0` - Soname symlink (ABI version 0)
- `libptywrap.so` - Development symlink

## Installation

```bash
sudo make install
```

This installs:
- `/usr/local/lib/libptywrap.so.0.1` - Real library file
- `/usr/local/lib/libptywrap.so.0` - Soname symlink
- `/usr/local/lib/libptywrap.so` - Development symlink
- `/usr/local/include/ptywrap.h` - Public API header

## Usage

### Basic Example

```c
#include <ptywrap.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    /* Attach to existing running container by ID or name */
    /* First start a container: podman run -d --name mytest alpine sleep 3600 */
    
    ptywrap_session_t *sess = ptywrap_create("mytest", 40, 150);
    if (!sess) {
        perror("Failed to attach to container");
        return 1;
    }

    sleep(1);  /* Wait for exec to attach */

    /* Send command to container */
    ptywrap_send_line(sess, "echo 'Hello from container'");
    sleep(1);  /* Wait for output */

    /* Read terminal buffer */
    char buf[256];
    for (int row = 0; row < 10; row++) {
        ptywrap_get_row_text(sess, row, buf, sizeof(buf));
        printf("Row %d: %s\n", row, buf);
    }

    /* Cleanup - container continues running */
    ptywrap_destroy(sess);
    return 0;
}
```

Compile:
```bash
gcc -o myapp myapp.c -lptywrap -pthread
```

Run:
```bash
LD_LIBRARY_PATH=. ./myapp
```

## API Reference

### Session Management

- `ptywrap_session_t* ptywrap_create(const char *container_image, int rows, int cols)`
  - Create new PTY session with container
  - Use 0 for rows/cols to get defaults (40x150)
  - Returns NULL on failure

- `void ptywrap_destroy(ptywrap_session_t *session)`
  - Destroy session and free resources
  - Does NOT stop the container

### Input Operations

- `int ptywrap_send(ptywrap_session_t *session, const char *data, size_t len)`
  - Send raw bytes to container
  - Returns bytes written or negative error code
  - Respects configured inter-character delay (see Configuration below)

- `int ptywrap_send_str(ptywrap_session_t *session, const char *str)`
  - Send null-terminated string

- `int ptywrap_send_line(ptywrap_session_t *session, const char *str)`
  - Send string with newline appended

### Configuration

- `int ptywrap_set_send_delay(ptywrap_session_t *session, int delay_ms)`
  - Configure inter-character delay for send operations (default: 0ms)
  - Useful for interactive applications like vi/vim that need time to process each keystroke
  - Negative values are treated as 0
  - Returns PTYWRAP_OK or error code

- `int ptywrap_get_send_delay(ptywrap_session_t *session)`
  - Get current inter-character delay setting
  - Returns delay in milliseconds, or negative error code

**Example**: For reliable vi input
```c
ptywrap_set_send_delay(sess, 10);  /* 10ms between characters */
ptywrap_send(sess, "iHello, World!", 14);  /* Enter insert mode and type */
ptywrap_set_send_delay(sess, 0);   /* Reset to normal speed */
```

### Output Operations

- `int ptywrap_get_size(ptywrap_session_t *session, int *rows, int *cols)`
  - Get terminal buffer dimensions

- `int ptywrap_get_cell(ptywrap_session_t *session, int row, int col, ptywrap_cell_t *cell)`
  - Get single cell with attributes

- `int ptywrap_get_row(ptywrap_session_t *session, int row, ptywrap_cell_t *cells)`
  - Copy entire row with attributes

- `int ptywrap_get_row_text(ptywrap_session_t *session, int row, char *buf, size_t buflen)`
  - Get row as plain text (no attributes)

- `int ptywrap_get_cursor(ptywrap_session_t *session, int *row, int *col)`
  - Get current cursor position

### Screenshot / Export

- `char* ptywrap_screenshot_markdown(ptywrap_session_t *session, int start_row, int end_row)`
  - Generate markdown representation of terminal buffer
  - Returns dynamically allocated string (caller must free)
  - Includes styling annotations for colors and attributes
  - Example: `[(bold)(fg:blue)filename]` for bold blue text

### Status Queries

- `int ptywrap_container_alive(ptywrap_session_t *session)`
  - Check if container is running (1 = running, 0 = stopped)

- `pid_t ptywrap_get_container_pid(ptywrap_session_t *session)`
  - Get container process ID

### Data Types

```c
typedef struct {
    char ch;              /* Character */
    uint8_t fg_color;     /* Foreground color (0-255) */
    uint8_t bg_color;     /* Background color (0-255) */
    uint8_t attrs;        /* Attribute flags */
} ptywrap_cell_t;
```

Attribute flags:
- `PTYWRAP_ATTR_BOLD`
- `PTYWRAP_ATTR_UNDERLINE`
- `PTYWRAP_ATTR_REVERSE`
- `PTYWRAP_ATTR_BLINK`

### Return Codes

- `PTYWRAP_OK` (0) - Success
- `PTYWRAP_ERR_INVAL` - Invalid argument
- `PTYWRAP_ERR_PTY` - PTY operation failed
- `PTYWRAP_ERR_CONTAINER` - Container spawn failed
- `PTYWRAP_ERR_THREAD` - Thread creation failed
- `PTYWRAP_ERR_NOMEM` - Memory allocation failed

## Examples

See `examples/` directory:
- `simple.c` - Basic usage demonstration
- `interactive.c` - Interactive shell session
- `screenshot.c` - Terminal screenshot to markdown

Build examples:
```bash
cd examples
gcc -o simple simple.c -I../include -L.. -lptywrap -pthread
gcc -o interactive interactive.c -I../include -L.. -lptywrap -pthread
```

## Python Bindings

Python bindings are available in the `python/` directory.

### Installation

**Prerequisites:** Install the C library first (`make && sudo make install`)

**From PyPI (recommended, when published):**
```bash
pip install ptywrap
```

**From source:**
```bash
cd python
pip install .
```

**Development mode:**
```bash
cd python
pip install -e .
```

### Usage

```python
import ptywrap
import time

session = ptywrap.PTYSession("mycontainer")
session.send_line("ls -la")
time.sleep(1)
text = session.get_row_text(0)
markdown = session.screenshot(0, 10)
```

See [python/README.md](python/README.md) for complete documentation.

## Testing

Basic test suite:
```bash
cd tests
gcc -o test_basic test_basic.c -I../include -L.. -lptywrap -pthread
LD_LIBRARY_PATH=.. ./test_basic
```

Integration test (vi editor):
```bash
gcc -o test_vi_edit test_vi_edit.c -I../include -L.. -lptywrap -pthread
LD_LIBRARY_PATH=.. ./test_vi_edit
```

This test:
- Installs vi in the container
- Creates and edits a hello.c file
- Saves the file and verifies content
- Compiles and runs the program

Note: Tests require podman and the alpine:latest image.

## VT100/ANSI Support

Supported escape sequences:
- **SGR (m)**: Colors, bold, underline, reverse
  - 8 basic colors (30-37, 40-47)
  - 256-color mode (38;5;n, 48;5;n)
- **Cursor movement**: Up (A), Down (B), Forward (C), Back (D)
- **Cursor positioning**: H, f
- **Erase**: Display (J), Line (K)

## Thread Safety

All public API functions are thread-safe. The internal terminal buffer is protected by a mutex.

## Container Lifecycle

The library does NOT create or destroy containers. It only attaches to existing running containers via `podman exec`.

To use the library:
1. Start a container: `podman run -d --name mycontainer alpine sleep 3600`
2. Attach to it: `ptywrap_create("mycontainer", 40, 150)`
3. When done: `ptywrap_destroy()` - container continues running
4. Stop container separately if needed: `podman stop mycontainer`

## Memory Management

The library uses standard C memory management:
- All allocations are freed in `ptywrap_destroy()`
- No memory leaks (verified with valgrind)

## Error Handling

Functions return:
- `PTYWRAP_OK` (0) or positive values on success
- Negative error codes on failure
- Check `errno` for system call errors

## Architecture

```
┌─────────────┐
│   User App  │
└──────┬──────┘
       │ ptywrap API
┌──────▼──────────────────────────┐
│  libptywrap.so                  │
│  ┌──────────┐  ┌─────────────┐ │
│  │ PTY Mgr  │  │  Terminal   │ │
│  │          │  │  Emulator   │ │
│  └────┬─────┘  └─────▲───────┘ │
│       │              │         │
│  ┌────▼──────────────┴───────┐ │
│  │   Reader Thread           │ │
│  └───────────────────────────┘ │
└─────────────┬───────────────────┘
              │ PTY
┌─────────────▼───────────────────┐
│   podman run -it <image>        │
│   ┌───────────────────────┐     │
│   │  /bin/sh              │     │
│   └───────────────────────┘     │
└─────────────────────────────────┘
```

## License

MIT License (or your preferred license)

## Author

Generated via Claude Code
