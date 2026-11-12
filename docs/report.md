# Project Report

## 9. Lexer Design

The lexer is implemented using Flex and C. It scans the input stream and returns tokens through the shared `Token` structure, which records token kind, source lexeme, one-based line and column, and converted numeric values where relevant. The scanner API is `lexer_set_input(FILE *)` followed by repeated calls to `lexer_next(Token *)` until `TOKEN_EOF`.

Explicit rules recognize the approved keywords and Boolean literals before the generic identifier rule. Flex's longest-match behavior distinguishes multi-character operators such as `==`, `!=`, `<=`, `>=`, `&&`, and `||` from their one-character forms. Identifiers use `[A-Za-z_][A-Za-z0-9_]*`; integers use `[0-9]+`; floats use `[0-9]+\.[0-9]+`. Scientific notation is reported as unsupported.

Whitespace, `//` comments, and `/* ... */` comments are discarded. A Flex start condition reads block comments and reports an unterminated comment at its opening location. The location tracker counts LF, CR, and CRLF correctly and retains token start positions. Invalid characters produce location-bearing lexical errors; scanning continues, and the standalone driver returns a nonzero status if any error was found.

The lexer was built from a clean build directory with GCC and Flex. Running `make test` executed eight focused cases; all eight passed. The test cases are under `tests/cases/`, with their expected output under `tests/expected/`.

## 10. Parser Design

The syntax analyzer is implemented in `src/parser/parser.y` with Bison. The parser grammar follows the approved statement and expression productions. Expression precedence is represented by separate grammar levels from `logical_or` down to `primary`, which gives `!` the tightest binding, followed by multiplicative, additive, relational, equality, `&&`, and `||`. Binary arithmetic and logical rules are left-recursive for left associativity. Relational and equality rules allow at most one comparison operator at their respective level, so chained comparisons are rejected.

`src/parser/lexer_adapter.c` maps the existing Flex `TokenKind` values to Bison tokens, transfers identifier and numeric values, and provides token source locations. Bison verbose errors are combined with the current lexeme and line/column. The parser has recovery points at semicolons and closing braces; any lexical or syntax error causes `parser_parse` to return failure without an AST.

The parser was built with the installed Bison 2.4.2 and GCC 6.3.0. The integrated `make test` run passed all eight lexer cases and all fourteen parser cases, including operator precedence, nested/control-flow statements, empty input, malformed syntax, lexical errors, recovery at a closing brace, and rejected comparison chains.

## 11. Abstract Syntax Tree

The AST is defined in `src/ast/ast.h` and constructed through functions in `src/ast/ast.c` during Bison reductions. `AstProgram` owns a statement list. Tagged `AstStmt` nodes represent declarations, assignments, blocks, conditionals, loops, and print statements. Tagged `AstExpr` nodes represent identifiers, integer/float/Boolean literals, binary operators, and unary logical NOT. Nodes preserve one-based source start locations. Parentheses affect expression grouping but do not create AST nodes.

The `ast_print` function displays an indented tree. For example, `age = 21 + 5;` appears as an assignment whose value is a `Binary (+)` node with integer children `21` and `5`. AST free functions recursively release child nodes, statement-list cells, and owned identifier strings. Bison destructors release semantic values discarded during error recovery; a parse containing any error does not return its AST to the caller.
