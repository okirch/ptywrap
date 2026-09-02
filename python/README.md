# PTYwrap Python Bindings

Python bindings for the ptywrap library - PTY container emulator with VT100/ANSI terminal emulation.

## Installation

### Prerequisites

The ptywrap C library must be installed first:

```bash
cd ..  # ptywrap repository root
make
sudo make install
sudo ldconfig
```

You also need Python development headers:
```bash
# Debian/Ubuntu
sudo apt-get install python3-dev

# Fedora/RHEL
sudo dnf install python3-devel

# openSUSE
sudo zypper install python3-devel
```

### Install Python Bindings

**From PyPI (recommended, when published):**
```bash
pip install ptywrap
```

**From source:**
```bash
pip install .
```

**Development mode:**
```bash
pip install -e .
```

**Legacy method (not recommended):**
```bash
python3 setup.py install
```

For detailed installation instructions and troubleshooting, see [INSTALL.md](INSTALL.md).

## Quick Start

```python
import ptywrap
import time

# Start a container first:
# podman run -d --name mytest alpine sleep 3600

# Create PTY session
session = ptywrap.PTYSession("mytest", rows=40, cols=150)

# Send commands
session.send_line("ls -la")
time.sleep(1)

# Read output
text = session.get_row_text(0)
print(text)

# Take a screenshot
markdown = session.screenshot(0, 10)
print(markdown)
```

## API Reference

### Class: PTYSession

```python
PTYSession(container_id: str, rows: int = 0, cols: int = 0)
```

Creates a PTY session attached to a running container.

**Parameters:**
- `container_id` - Container ID or name (must be running)
- `rows` - Terminal height (default: 40)
- `cols` - Terminal width (default: 150)

**Properties:**
- `rows` - Number of terminal rows (read-only)
- `cols` - Number of terminal columns (read-only)

### Methods

#### send(data: bytes) -> int

Send raw bytes to the container shell.

```python
bytes_written = session.send(b"ls\\n")
```

#### send_line(line: str) -> int

Send a line of text with newline appended.

```python
session.send_line("echo hello")
```

#### get_cell(row: int, col: int) -> dict

Get terminal cell at specific position.

Returns dictionary with:
- `char` - Character at position
- `fg_color` - Foreground color (0-255)
- `bg_color` - Background color (0-255)
- `attrs` - Attribute flags (bold, underline, etc.)

```python
cell = session.get_cell(0, 0)
print(f"Character: {cell['char']}")
if cell['attrs'] & ptywrap.ATTR_BOLD:
    print("Bold text!")
```

#### get_row_text(row: int) -> str

Get text content of a row (without attributes).

```python
text = session.get_row_text(5)
print(text)
```

#### get_cursor() -> tuple[int, int]

Get current cursor position.

```python
row, col = session.get_cursor()
print(f"Cursor at ({row}, {col})")
```

#### screenshot(start_row: int = 0, end_row: int = -1) -> str

Generate markdown representation of terminal buffer.

```python
# Full terminal
markdown = session.screenshot()

# Specific rows
markdown = session.screenshot(0, 10)

# Save to file
with open("screenshot.md", "w") as f:
    f.write(markdown)
```

#### is_alive() -> bool

Check if the exec process is still running.

```python
if session.is_alive():
    print("Still running")
```

#### get_pid() -> int

Get PID of the podman exec process.

```python
pid = session.get_pid()
print(f"Process ID: {pid}")
```

## Constants

- `ptywrap.ATTR_BOLD` - Bold text attribute
- `ptywrap.ATTR_UNDERLINE` - Underline attribute
- `ptywrap.ATTR_REVERSE` - Reverse video attribute
- `ptywrap.ATTR_BLINK` - Blink attribute

## Example

See `example.py` for a complete example:

```bash
# Start container
podman run -d --name mytest alpine sleep 3600

# Run example
python3 example.py
```

## Error Handling

```python
import ptywrap

try:
    session = ptywrap.PTYSession("nonexistent")
except RuntimeError as e:
    print(f"Failed to attach: {e}")

try:
    text = session.get_row_text(999)  # Out of bounds
except ValueError as e:
    print(f"Invalid row: {e}")
```

## Type Hints

The module includes comprehensive docstrings that work with Python's `help()` function:

```python
import ptywrap
help(ptywrap.PTYSession)
help(ptywrap.PTYSession.screenshot)
```

## License

GPL 2.0 or later

## See Also

- [ptywrap C library documentation](../README.md)
- [Example code](example.py)
