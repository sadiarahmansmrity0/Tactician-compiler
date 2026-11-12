# Parser and Abstract Syntax Tree Design

## What syntax analysis does

The lexer recognizes tokens; the parser checks whether their order forms a Tactician program. A **context-free grammar** describes valid groupings of tokens. Each grammar statement such as `assignment -> IDENTIFIER '=' expression ';'` is a **production rule**: it says what forms can be used for an assignment.

A parser can internally describe the full derivation as a parse tree, including every grammar helper such as `logical_or` and `multiplicative`. The compiler instead keeps an **abstract syntax tree (AST)**: a smaller tree containing the meaningful constructs. Parentheses and grammar-only grouping nodes do not need separate AST nodes; their effect remains in the tree's shape.

For example, `age = 21 + 5;` becomes an assignment node whose value is a `+` binary-expression node with integer children. This tree directly describes the operation and is easier for future semantic analysis and TAC generation to walk.

## Bison and the parser grammar

Bison reads `src/parser/parser.y` and generates a C parser in `build/parser.tab.c` plus token declarations in `build/parser.tab.h`. At runtime, the parser requests one token at a time through `yylex`, then applies grammar productions to recognize statements and expressions.

The parser grammar follows the approved tiers: statement, declaration/assignment/control/print/block, then expression levels from `logical_or` through `primary`. The implementation uses a left-recursive statement list for Bison's efficient list parsing; it accepts the same zero-or-more sequence described by the CFG. The grammar layers encode precedence, so `a + b * c` becomes `+(a, *(b,c))`. Left-recursive arithmetic and logical productions make binary operations left-associative, so `a - b - c` becomes `-(-(a,b),c)`. Relational and equality productions have only zero-or-one operator, preventing `a < b < c` and `a == b == c` while allowing `a < b && b < c`.

No precedence declarations are needed because precedence is expressed directly by the grammar tiers. Unary `!` recurses into another unary expression and therefore binds more tightly than multiplication. Unary minus is not in the grammar.

## Lexer-to-parser connection

The scanner's public API remains `lexer_next(Token *)`. `src/parser/lexer_adapter.c` provides Bison's `yylex()` function and maps each existing `TokenKind` to its Bison token. It transfers identifier text and numeric values into Bison's semantic-value union and copies the token's line/column into Bison's `yylloc` location structure. The Flex scanner's internal symbols use the `raw_` prefix so its scan function does not collide with the adapter's `yylex`.

The adapter reports and skips lexical-error tokens so scanning can continue. It counts those errors; even if the remaining tokens parse, the parser API rejects and frees the AST when any lexical error occurred.

## AST node structure and construction

AST definitions and constructors are in `src/ast/ast.h` and `src/ast/ast.c`.

- `AstProgram` owns a statement list.
- `AstStmt` is tagged as declaration, assignment, block, if, if-else, while, or print. Its union holds just the fields for that kind.
- `AstExpr` is tagged as identifier, integer, float, Boolean, binary, or unary. Binary expressions hold an operator and two child expressions; unary NOT holds one child.
- Each statement/expression node stores the one-based start `SourceLocation`. Statement-list cells also retain their statement's location.

Bison production actions are **semantic actions**: small C snippets run when a production is recognized. Here they call AST constructors and connect children. For example, the action for `additive '+' multiplicative` creates a binary `AST_OP_ADD` node. These actions do not perform semantic checks or generate TAC.

Constructors take ownership of successfully attached child nodes and identifier strings. AST destructors recursively free those children, list cells, and owned names. Bison `%destructor` rules free semantic values that are discarded during syntax-error recovery. On parse failure, the parser API frees any recovered root and returns no AST.

## AST display

`ast_print` writes an indented tree. For:

```text
int age;
age = 21 + 5;
print age;
```

the structure is shown as:

```text
Program
  Declaration (int age)
  Assignment (age)
    Binary (+)
      Integer (21)
      Integer (5)
  Print
    Identifier (age)
```

## Syntax errors and recovery

Bison's verbose syntax messages include the unexpected token and, where available, expected tokens. The adapter adds the current lexeme and line/column. The grammar synchronizes at semicolons within statement lists and at closing braces within blocks. Recovery is for continued diagnostics; any syntax error still makes `parser_parse` return failure and no AST to its caller.

## Build and test

```sh
make parser
make test
make clean
```

Run the parser/AST display on a source file:

```sh
build/tactician-parser.exe tests/parser/valid/simple.tac
```

`make test` reruns the standalone lexer suite and parser cases. Parser valid/invalid inputs live in `tests/parser/valid/` and `tests/parser/invalid/`; expected output is under `tests/parser/expected/`.
