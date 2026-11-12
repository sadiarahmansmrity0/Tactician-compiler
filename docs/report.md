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

## 12. Semantic Analysis

The semantic analyzer in `src/semantic/` walks the parsed AST in source order using a fresh symbol table. Declarations become visible when their declaration nodes are visited, so an earlier assignment or expression use cannot see a later declaration. Each block enters a scope and restores its parent scope when traversal leaves the block; this includes blocks used by `if`, `if-else`, and `while`. Child-scope shadowing is preserved, and duplicate declarations in one scope are reported.

Expression analysis recursively returns `int`, `float`, `bool`, or an internal error type. It enforces numeric arithmetic, integer-only modulo, numeric relational comparisons, compatible equality, Boolean logical operators and NOT, approved assignment widening, and Boolean control-flow conditions. `print` accepts any correctly typed expression. Errors include source lines; analysis continues across independent statements and suppresses type follow-ons when an operand is already invalid.

The public `semantic_analyze` API returns a success flag and error count and writes diagnostics to a caller-provided stream. The semantic command-line driver first requires successful parsing, then runs semantic analysis. A clean integrated test run passed all eight lexer cases, fourteen parser cases, three symbol-table harness cases, and twenty-four semantic cases (six valid and eighteen invalid), with no failures.

## 13. Symbol Table

The symbol table in `src/symbol_table/` stores each declared name, its `int`, `float`, or `bool` type, scope ID, and declaration line. Scopes form a retained parent/child tree with the program's outer scope at ID 0. Lookup starts at the current scope and walks outward; local lookup checks only the current scope. A declaration is rejected as a duplicate only if its name already exists in the current scope.

The project permits shadowing: a nested scope may declare a name already present in a parent. The nested declaration is found while that scope is active; after exiting, lookup resolves the parent declaration again. Blocks in the AST create scopes, including nested blocks and the blocks used as `if` and `while` bodies. Exited scopes stay in the tree for table display, but ordinary lookup does not search exited siblings or descendants.

`symbol_table_build_from_ast` traverses declarations and block statements, reports duplicates with line and scope, and provides a readable scope-tree dump. The table retains a borrowed association between AST blocks and their built scopes; a later AST walk can re-enter a matching block with `symbol_table_enter_existing_block`. The build helper gathers declarations before semantic analysis and therefore does not enforce declaration-before-use. The next semantic pass should populate a fresh table in source order as it visits declarations and expressions.

The integrated `make test` run passed eight lexer cases, fourteen parser cases, and all three symbol-table harness cases (the API unit checks and two AST-based table snapshots). The tests cover all three types, lookup and missing names, same-scope duplicates, nested shadowing, parent visibility, scope exit/restoration, AST scope creation for blocks/conditionals/loops, and readable table output.
