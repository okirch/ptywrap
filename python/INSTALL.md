# Installation Guide for ptywrap Python Bindings

## Prerequisites

The Python bindings require the ptywrap C library to be installed on your system.

### Install C Library

```bash
# From the ptywrap repository root
cd ptywrap
make
sudo make install
sudo ldconfig
```

This installs:
- `/usr/local/lib/libptywrap.so.*` - Shared library
- `/usr/local/include/ptywrap.h` - Header file

## Installation Methods

### Method 1: Install from PyPI (when published)

```bash
pip install ptywrap
```

### Method 2: Install from Source

```bash
# Clone repository
git clone https://github.com/openSUSE/ptywrap.git
cd ptywrap/python

# Install
pip install .
```

### Method 3: Development Installation

For development, use editable mode:

```bash
cd ptywrap/python
pip install -e .
```

Changes to Python code will be immediately available without reinstalling.

## Verification

Test the installation:

```bash
python3 -c "import ptywrap; print('ptywrap installed successfully')"
```

Run the example:

```bash
# First, start a test container
podman run -d --name mytest alpine sleep 3600

# Then run the example
python3 -c "
import ptywrap
import time

sess = ptywrap.PTYSession('mytest')
sess.send_line('echo Hello from Python!')
time.sleep(1)
print(sess.get_row_text(1))
"
```

## Troubleshooting

### ImportError: libptywrap.so.0: cannot open shared object file

The C library is not installed or not in the library path.

**Solution:**
```bash
# Verify library is installed
ls -la /usr/local/lib/libptywrap.so*

# If not found, install it:
cd ptywrap
sudo make install
sudo ldconfig

# If installed but not found, add to LD_LIBRARY_PATH:
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Build fails with "ptywrap.h: No such file or directory"

The header file is not in the expected location.

**Solution:**
```bash
# Verify header is installed
ls -la /usr/local/include/ptywrap.h

# If not found, install the C library:
cd ptywrap
sudo make install
```

### Development install: Changes to C library not reflected

When developing the C library, you need to rebuild and reinstall it.

**Solution:**
```bash
# Rebuild and reinstall C library
cd ptywrap
make clean && make
sudo make install
sudo ldconfig

# The Python bindings will automatically pick up the new version
```

## Uninstallation

```bash
pip uninstall ptywrap
```

To also remove the C library:

```bash
sudo rm -f /usr/local/lib/libptywrap.so*
sudo rm -f /usr/local/include/ptywrap.h
sudo ldconfig
```

## Platform Support

- **Linux**: Fully supported
- **macOS**: Not supported (requires Linux-specific PTY features)
- **Windows**: Not supported (requires POSIX PTY and podman)

## Dependencies

- Python >= 3.6
- libptywrap (C library)
- podman (for container operations)
- GCC with C11 support (for building from source)
