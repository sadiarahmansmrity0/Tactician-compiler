# Semantic Analyzer Design

## Purpose

The parser checks whether tokens form a legal Tactician program. Semantic analysis checks whether that parsed program makes sense: names must be declared before use, assignments must be type-compatible, and operators and conditions must receive the right types.

## Public API

`src/semantic/semantic.h` provides:

```c
typedef struct SemanticResult {
    int success;
    int error_count;
} SemanticResult;

SemanticResult semantic_analyze(const AstProgram *program, FILE *diagnostics);
```

The analyzer creates and destroys its own fresh symbol table. It writes one human-readable diagnostic per semantic error to the supplied stream (or `stderr` when the stream is `NULL`). The result reports whether analysis succeeded and how many errors it found.

## Source-order AST walk and scopes

The analyzer visits the AST statement list in its stored source order. It does not use `symbol_table_build_from_ast`, because that helper pre-collects declarations and would incorrectly make a later declaration visible at an earlier use.

- A declaration is inserted when the walk reaches it.
- An assignment resolves its target and analyzes its value at that point in the source.
- An identifier expression looks up its name in the current scope and enclosing scopes.
- A block enters a new symbol-table scope before its statements and exits afterward.
- `if`, `if-else`, and `while` visit their block children; those blocks create and leave their own scopes.

The symbol table's current-scope lookup supports local shadowing. A local declaration takes effect from its declaration onward; before it, lookup can still find a parent declaration. Exiting a block restores parent lookup and hides local names. Same-scope duplicates are reported and the earlier declaration remains the active binding.

## Expression type checking

The recursive expression analyzer returns one of `int`, `float`, `bool`, or an internal error type. Literal nodes provide their type directly. Identifiers get their type from the symbol table. Unary and binary expression nodes check their operands and return a result type.

| Expression | Accepted types | Result |
|---|---|---|
| `+`, `-`, `*`, `/` | Numeric operands (`int` and/or `float`) | `int` if both are `int`; otherwise `float` |
| `%` | `int`, `int` | `int` |
| `<`, `>`, `<=`, `>=` | Numeric operands | `bool` |
| `==`, `!=` | Same types, or mixed numeric types | `bool` |
| `&&`, `||` | `bool`, `bool` | `bool` |
| `!` | `bool` | `bool` |

Assignments permit identical types and `int` expressions assigned to `float` variables. `float` to `int` and Boolean/numeric assignments are rejected. `if` and `while` conditions must be Boolean. `print` accepts any expression whose analysis succeeds.

These are the project's approved compatibility rules from `docs/grammar.md` and `docs/decisions.md`; semantic analysis does not change the language specification.

## Error reporting and recovery

Each message includes the relevant source line. The analyzer continues with later statements after a semantic error. If an expression contains an undeclared identifier or an invalid child expression, it returns the internal error type; enclosing operators and assignments avoid emitting misleading follow-on type errors for that expression. Independent errors in other statements are still reported.

The analyzer handles every current AST statement and expression kind explicitly. If an unrecognized AST kind is encountered, it reports an unsupported-node diagnostic instead of silently skipping it.

## Build and tests

Build the semantic command-line analyzer:

```sh
make semantic
```

Analyze a source file:

```sh
build/tactician-semantic.exe tests/semantic/valid/basic.tac
```

Run the full lexer, parser, symbol-table, and semantic suites:

```sh
make test
```

Semantic valid/invalid programs are under `tests/semantic/valid/` and `tests/semantic/invalid/`; expected diagnostics/results are under `tests/semantic/expected/`. The test script compares complete output and exit status.

## Current limitations

The analyzer reports errors and an aggregate result but does not store inferred expression types on the AST or return a type map. Later TAC generation may need to repeat type queries or introduce a separate annotation/result structure. The implementation does not generate TAC or perform runtime checks such as division-by-zero detection.
