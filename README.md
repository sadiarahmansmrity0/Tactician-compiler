# Tactician Compiler 🚀
A custom mini-programming language compiler front-end with Three Address Code (TAC) generation, built using Flex, Bison, and C.

- **Course:** Compiler Construction Lab (CSE)
- **Institution:** Metropolitan University, Bangladesh
- **Team Members:** Sadia Rahman - (232-115-092)
                    Tabassum zaman - (232-115-098)

## Build and run

The integrated command-line compiler builds with the project toolchain (GCC,
Flex, Bison, and Make):

```sh
make
```

To compile a Tactician source file and print its Three-Address Code:

```sh
build/tactician-tac.exe path/to/program.tac
```

The driver performs lexical and syntax analysis, builds the AST, runs semantic
analysis, and emits TAC only when the preceding stages succeed. Run all module
and end-to-end tests with `make test`; remove generated build files with
`make clean`.
