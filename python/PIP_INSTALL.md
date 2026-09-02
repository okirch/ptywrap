# Quick Start: Installing ptywrap with pip

## For Users

### Install from PyPI (when published)

```bash
pip install ptywrap
```

**Note:** The C library (`libptywrap`) must be installed on your system first.

### Install C Library

```bash
# Download and build ptywrap
git clone https://github.com/openSUSE/ptywrap.git
cd ptywrap
make
sudo make install
sudo ldconfig
```

Then install Python bindings:

```bash
pip install ptywrap
```

## For Developers

### Development Installation

```bash
# Clone repository
git clone https://github.com/openSUSE/ptywrap.git
cd ptywrap

# Build and install C library
make
sudo make install
sudo ldconfig

# Install Python bindings in development mode
cd python
pip install -e .
```

### Without Installing C Library

For development without system installation:

```bash
# Build C library (don't install)
cd ptywrap
make

# Install Python bindings (finds library in parent dir)
cd python
pip install -e .

# When running, ensure library is in path
export LD_LIBRARY_PATH=$(pwd)/..:$LD_LIBRARY_PATH
python3 your_script.py
```

## Quick Test

```bash
python3 << 'EOF'
import ptywrap
print("ptywrap version:", getattr(ptywrap, '__version__', 'development'))
print("Available classes:", [x for x in dir(ptywrap) if not x.startswith('_')])

# Start a test container first:
# podman run -d --name test-container alpine sleep 3600

# Then test:
# session = ptywrap.PTYSession('test-container')
# session.send_line('echo Hello!')
# print(session.get_row_text(1))
EOF
```

## Common Issues

### "ImportError: libptywrap.so.0: cannot open shared object file"

**Cause:** C library not installed or not in library path

**Solution:**
```bash
sudo make install
sudo ldconfig
```

Or for development:
```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### "ModuleNotFoundError: No module named 'ptywrap'"

**Cause:** Python package not installed

**Solution:**
```bash
pip install ptywrap
```

### Build fails: "ptywrap.h: No such file or directory"

**Cause:** C library headers not installed

**Solution:**
```bash
cd ptywrap
sudo make install
```

## More Information

- Full installation guide: [INSTALL.md](INSTALL.md)
- Python API documentation: [README.md](README.md)
- Packaging for PyPI: [PACKAGING.md](PACKAGING.md)
