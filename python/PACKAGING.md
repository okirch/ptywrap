# Packaging Guide for PyPI Distribution

This document describes how to build and publish the ptywrap Python package to PyPI.

## Prerequisites

1. Install build tools:
```bash
pip install --upgrade build twine
```

2. Ensure the C library is built and installed:
```bash
cd ..
make
sudo make install
sudo ldconfig
```

## Building the Package

### Build Source Distribution and Wheel

**Important:** The C library must be installed to system paths (`/usr/local/lib`) before building, as `python -m build` runs in an isolated environment.

```bash
# Install C library first
cd ..
make
sudo make install
sudo ldconfig

# Now build Python package
cd python
python3 -m build
```

This creates:
- `dist/ptywrap-0.1.0.tar.gz` - Source distribution
- `dist/ptywrap-0.1.0-cp311-cp311-linux_x86_64.whl` - Binary wheel

**Alternative:** Use pip without build isolation:
```bash
pip install . --no-build-isolation
```

This allows pip to find the library in the parent directory without system installation.

### Verify the Build

```bash
# Check package contents
tar -tzf dist/ptywrap-0.1.0.tar.gz

# Verify wheel
unzip -l dist/ptywrap-0.1.0-*.whl
```

### Test Installation

```bash
# Create virtual environment
python3 -m venv test_env
source test_env/bin/activate

# Install from wheel
pip install dist/ptywrap-0.1.0-*.whl

# Test import
python3 -c "import ptywrap; print('Success!')"

# Cleanup
deactivate
rm -rf test_env
```

## Publishing to PyPI

### Test PyPI (Recommended First)

1. Register at https://test.pypi.org/
2. Create API token
3. Upload:

```bash
python3 -m twine upload --repository testpypi dist/*
```

4. Test installation:
```bash
pip install --index-url https://test.pypi.org/simple/ ptywrap
```

### Production PyPI

1. Register at https://pypi.org/
2. Create API token
3. Upload:

```bash
python3 -m twine upload dist/*
```

4. Verify:
```bash
pip install ptywrap
```

## Version Management

Update version in two places before releasing:

1. `setup.py`:
```python
version='0.1.0',
```

2. `pyproject.toml`:
```toml
version = "0.1.0"
```

## Creating a Release

1. Update version numbers
2. Update CHANGES.md with release notes
3. Commit changes:
```bash
git add setup.py pyproject.toml ../CHANGES.md
git commit -m "Bump version to 0.1.0"
git tag -a v0.1.0 -m "Release version 0.1.0"
```

4. Build package:
```bash
rm -rf dist/
python3 -m build
```

5. Upload to PyPI:
```bash
python3 -m twine upload dist/*
```

6. Push to git:
```bash
git push origin main --tags
```

## Package Configuration

### setup.py
- Defines build process
- Links against installed libptywrap.so
- Includes metadata for PyPI

### pyproject.toml
- Modern Python packaging metadata
- Build system requirements
- Project URLs and classifiers

### MANIFEST.in
- Controls which files are included in source distribution
- Includes README, examples, etc.

## Troubleshooting

### "libptywrap.so not found" during build

The C library must be installed before building the Python package.

**Solution:**
```bash
cd ..
make
sudo make install
sudo ldconfig
```

### Build fails with compiler errors

Ensure Python development headers are installed:

```bash
# Debian/Ubuntu
sudo apt-get install python3-dev

# Fedora/RHEL
sudo dnf install python3-devel

# openSUSE
sudo zypper install python3-devel
```

### Wheel is platform-specific

The wheel includes compiled C extension, so it's platform-specific. For wider distribution, consider:

1. Build on multiple platforms (Linux, various distros)
2. Use cibuildwheel for automated multi-platform builds
3. Provide source distribution for users to build locally

## Security

- Never commit API tokens to git
- Use PyPI API tokens instead of passwords
- Store tokens in `~/.pypirc`:

```ini
[pypi]
username = __token__
password = pypi-...

[testpypi]
username = __token__
password = pypi-...
```

Set permissions:
```bash
chmod 600 ~/.pypirc
```

## References

- [Python Packaging Guide](https://packaging.python.org/)
- [PyPI Documentation](https://pypi.org/help/)
- [Setuptools Documentation](https://setuptools.pypa.io/)
