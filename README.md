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

## Modules

### Preprocessor

The preprocessor iterates a directory of files, filters their content, and concatenates it in a `<dirname>.concat` output file. It can notably remove or normalize spaces and newlines, and remove c-style (non-quoted and non-escaped) comments. It also generates file and line mappings - data that is later used to find the actual source of a character from its position in the concatenated file.

### Findrepset

This module performs the actual clone detection in the concatenated file, and outputs a `<dirname>.output.txt` with the results.

This tool was not created as part of the project, but rather adapted from existing research. The documentation can be found as part of the following papers:

- Efficient repeat finding in sets of strings via suffix arrays
   P Barenbaum, V Becher, A Deymonnaz, M Halsband, PA Heiber, 2013.
   Discrete Mathematics and Theoretical Computer Science. 15(2):59-70

- (In Spanish) Melisa Halsband, Tesis de Licenciatura en Ingeniería,  Universidad de Buenos Aires. "Métodos Eficientes para la Identificación de Patrones en Conjuntos de Señales Discretas", Dirección: Verónica  Becher y Rosa Wachenchauzer, Diciembre 2010

- Efficient repeat finding via suffix arrays
   V Becher, A Deymonnaz, PA Heiber, 
   2013.

### Postprocessor

This module takes in the output from Findrepset as well as the line and file mappings from the preprocessor, and generates a file with all the repeated sequences and their locations in the source. As part of the processing, repeated sequences that span multiple files are also split along the file endings.

#### Format of the results

Each repeated sequence is on its own line, encoded as a top-level JSON object with 2 fields:

- `text`: the repeated byte sequence encoded as an escaped string
- `locations`: an array containing two or more objects with 3 fields each:
  - `path`: the path to the original source file in which the sequence was found
  - `start_line`: the line in the original source file at which the sequence started
  - `end_line`: the line in the original source file at which the sequence ended

### Notebooks

This project includes Python (jupyter) notebooks with scripts to process the files emitted by the postprocessor.