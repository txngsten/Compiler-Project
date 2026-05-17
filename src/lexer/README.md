# Lexical Analyser Artifact 1 COMP3722
This file will contain all details on how to navigate the repo and build the lexer.
First all the test inputs documented in the report are found in [testing](testing/).
As well as the unit test and its dependencies found in the [unit_test](unit_test/) directory.
Build system is using make via a [makefile](makefile), and the docker file is found in the [.build](.build/) directory located at the project root.

## Setup
For reproducible results with all dependencies automatically installed first ensure your machine has the latest version of [docker](https://www.docker.com/) installed.

To build the container ensure you have the docker engine running and are currently in the root directory of the project.
```bash
docker build -t lexer .build/
```
The enter the shell for the built container via:
```bash
docker run --rm -it -v "$(pwd)":/project lexer sh
```
Once in the shell the following make commands can be executed:
```bash
make lexer
```
Will build the lexer normally with the preset compiler flags.
```bash
make test 
```
Will build the unit test binary.
```bash
make clean
```
Will clean up any existing lexer or unit test binaries.

## Usage
Now for usage to run the lexer with a given input file use:
```bash
./lexer <input file name>
```
Similarly for the unit testing executable:
```bash
./unit_test/tests
```
For normal usage the output will be written to a tokens.txt file located in the relative directory to execution.

For checking the binary size within the container run (ensure the binary is compiled and exists):
```bash
stat -c %s lexer
```
If you don't build with container and specific compiler flags specified in the makefile reproducibility of results seen in the testing section of the report may not be possible.


