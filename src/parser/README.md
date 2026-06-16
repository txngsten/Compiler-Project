# Artifact 2 LR Parser, Theory and Practice of Computation (COMP3722) - Oliver Wuttke (WUTT0019)
This readme file is to help build, run, and navigate my parser project for COMP3722.

## Requirements
- A C++20-capable compiler (g++ ≥ 10 or clang++ ≥ 11)
- A C11-capable compiler for the lexer (gcc or clang)
- CMake ≥ 3.16 (only for the CMake build path)

## How to Build
For CMake (recommended) use the following:
```bash
cmake -S . -B build
cmake --build build
```
This will output both `build/lexer` and `build/parser`. 
If you want you can compile directly not recommended, the burden will be on you to ensure it works.

## How to Run
To get the tokenized output for a given source file from the lexer run:
```bash
./build/lexer <source-file>
```
This will output a `tokens.txt` file into your current directory.
Then to run the parser use:
```bash
./build/parser tokens.txt <desired-output-filename>
```
If no second argument is given it will automatically write the parse tree to parse_tree.txt.

The [test_file](test_files) directory contains all the test source files used in the report, results are reproducible using these files.
