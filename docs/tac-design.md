# TAC Generator Design

## Purpose

Three-Address Code (TAC) is an intermediate representation between the source
language and a later target-code stage. Each instruction is kept small, usually
with one operation and at most two operands. Tactician prints this IR for
inspection; this module does not generate assembly or executable files.

## Interface and validation

`tac_generate(const AstProgram *, FILE *)` in `src/tac/tac.h` walks an AST and
writes TAC to the supplied stream. It does not change the AST. The caller must
first validate the program. The command-line driver parses the source, runs
`semantic_analyze`, and only then calls `tac_generate`. A semantic failure
returns before TAC output is opened, so invalid programs cannot produce partial
TAC.

## Values and temporaries

Identifiers are printed by name, integers as decimal digits, floating values
using a compact decimal representation, and Booleans as `true` or `false`.
Declarations have no TAC instruction because this project has no runtime
storage backend. Assignments and print statements retain their source order.

Every computed arithmetic or Boolean result receives a temporary (`t1`, `t2`,
...). A fresh generator context starts numbering at `t1` for every call. For
example, `c = a + b * 2;` emits `t1 = b * 2`, `t2 = a + t1`, then `c = t2`.

## Expressions and short-circuit logic

Arithmetic operators emit one operation per temporary. Relational and equality
expressions produce a Boolean temporary initialized to `false`; branches set it
to `true` when the comparison succeeds. Boolean values use the textual literals
`true` and `false`.

`&&` and `||` are translated using conditional branches. The right operand of
`&&` is reached only when the left operand is true; the right operand of `||`
is reached only when the left operand is false. `!` swaps the true and false
destinations when used as a condition, and emits a Boolean operation when its
value must be stored.

## Control flow

Labels (`L1`, `L2`, ...) and temporaries are counted independently in one
per-generation context. If statements branch around their selected blocks; an
if-else also jumps over the else branch after the then branch. A while loop
places its condition at a test label, branches to its body or exit label, and
jumps back to the test label after the body. Nested blocks are traversed in
source order and do not add runtime instructions.

## Build and tests

Build the driver with `make tac`, then run it with a source file:

```text
make tac
build/tactician-tac.exe path/to/program.tac
```

`make test` runs lexer, parser, symbol-table, semantic, and TAC tests. TAC tests compare exact output for valid programs and verify
semantic diagnostics plus an empty TAC output for invalid programs.

## Limitations

TAC is a readable textual IR, not executable code. It has no storage layout,
runtime, register allocation, optimization, or machine-code backend. Floating
literal printing follows the host C library's `%.15g` formatting, so the text
is intended for readable intermediate output rather than round-trip source
serialization.
