# Code Repeats

Code Repeats is a clone detector for large sets of files.
It can be used with both text repositories and binaries.

## Prerequisites
- Cmake (build tool)
- zlib (compression library)
- Python 3

## Using the tool
1. Build the C++ binaries using cmake
2. Run `code_repeat.py path/to/scanned_repository`.
If the cmake output directory is not the working directory,
add `--prefix=path/to/build` to the command line.

The script has many options to configure the scan. They can be listed
with the `-h` argument.