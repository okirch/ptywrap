#!/usr/bin/env python3
"""
Setup script for ptywrap Python bindings

Prerequisites:
    libptywrap must be installed on the system:
        cd .. && make && sudo make install

Install from PyPI:
    pip install ptywrap

Install from source:
    pip install .

Development install:
    pip install -e .
"""

from setuptools import setup, Extension
import os

def get_readme():
    """Read README.md if available"""
    parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    readme_path = os.path.join(parent_dir, 'README.md')
    if os.path.exists(readme_path):
        with open(readme_path, 'r', encoding='utf-8') as f:
            return f.read()
    return 'Python bindings for ptywrap - PTY container emulator library'

def get_library_paths():
    """Determine library and include paths"""
    # Check for installed library first
    if os.path.exists('/usr/local/lib/libptywrap.so'):
        return {
            'include_dirs': ['/usr/local/include'],
            'library_dirs': ['/usr/local/lib'],
            'runtime_library_dirs': ['/usr/local/lib'],
        }

    # Fall back to parent directory for development
    parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if os.path.exists(os.path.join(parent_dir, 'libptywrap.so')):
        return {
            'include_dirs': [os.path.join(parent_dir, 'include')],
            'library_dirs': [parent_dir],
            'runtime_library_dirs': [parent_dir],
        }

    # Default to system paths
    return {
        'include_dirs': ['/usr/local/include'],
        'library_dirs': ['/usr/local/lib'],
        'runtime_library_dirs': ['/usr/local/lib'],
    }

# Extension links against libptywrap.so
lib_paths = get_library_paths()
ptywrap_module = Extension(
    'ptywrap',
    sources=['ptywrap_module.c'],
    libraries=['ptywrap'],
    extra_compile_args=['-std=c11'],
    **lib_paths
)

setup(
    name='ptywrap',
    version='0.1.0',
    description='Python bindings for ptywrap - PTY container emulator',
    long_description=get_readme(),
    long_description_content_type='text/markdown',
    author='SUSE Linux',
    author_email='okir@suse.com',
    url='https://github.com/openSUSE/ptywrap',
    license='GPL-2.0-or-later',
    ext_modules=[ptywrap_module],
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License v2 or later (GPLv2+)',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: C',
        'Topic :: Software Development :: Testing',
        'Topic :: System :: Emulators',
        'Operating System :: POSIX :: Linux',
    ],
    keywords='pty tty terminal container podman testing emulator',
    project_urls={
        'Bug Reports': 'https://github.com/openSUSE/ptywrap/issues',
        'Source': 'https://github.com/openSUSE/ptywrap',
        'Documentation': 'https://github.com/openSUSE/ptywrap/blob/main/README.md',
    },
)
