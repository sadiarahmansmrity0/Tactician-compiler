# Finalized Language Decisions

The Compiler Construction Lab Manual remains authoritative for required project features. This log records approved choices for details the manual does not specify. The matching formal grammar and semantic compatibility table are maintained in [grammar.md](grammar.md).

## Compiler toolchain

- Compiler core: C with Flex, Bison, GCC, and Make.
- Do not maintain separate C and C++ compiler implementations.
- Java UI is a supporting component under `ui/` on the separate `feature/ui` branch. UI implementation and integration are deferred until the command-line compiler is stable.

## Language details

- Support both `//` single-line comments and `/* ... */` block comments; the lexer discards them.
- Support logical NOT (`!`). Unary minus is excluded at this stage.
- Use conventional precedence: `!`, `* / %`, `+ -`, relational, equality, `&&`, `||` (highest to lowest).
- Binary arithmetic and logical operators are left-associative. Relational and equality operators do not chain.
- Integer literals contain digits; floating-point literals use digits-dot-digits. Scientific notation is unsupported.
- Permit one variable per declaration; declaration initialization is unsupported.
- Permit empty programs, empty blocks, and nested blocks.
- Integer division produces `int`.
- Mixed `int`/`float` arithmetic and equality are supported through promotion to `float`.
- Boolean equality (`==`, `!=`) is supported.
- `if` and `while` conditions must evaluate to `bool`.
- Modulo accepts integer operands only.
- Assignment permits widening from `int` to `float`; narrowing from `float` to `int` is rejected.

These language decisions supplement the manual where its specification leaves exact behavior open. They do not add features such as arrays, functions, declaration initialization, or unary minus.

## Compiler phase behavior

- Preserve source locations for useful diagnostics.
- Do not generate TAC if lexical, syntax, or semantic errors occur.
- Keep the Google Sheets prompt log separate from repository project documentation.
