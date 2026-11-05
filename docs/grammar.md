# Tactician Compiler — Context-Free Grammar (CFG)

This document specifies the formal Context-Free Grammar (CFG) in Backus-Naur Form (BNF) for the **Tactician** programming language, adhering strictly to Section 5 of the Compiler Construction Lab project manual.

## 1. Program Structure
program         -> statement_list

statement_list  -> statement statement_list
                 | /* epsilon / empty */

statement       -> var_decl
                 | assign_stmt
                 | if_stmt
                 | while_stmt
                 | print_stmt
                 | block

## 2. Declarations & Scoping Blocks
var_decl        -> type IDENTIFIER ';'

type            -> INT
                 | FLOAT
                 | BOOL

block           -> '{' statement_list '}'

## 3. Control Flow Statements
if_stmt         -> IF '(' expression ')' block
                 | IF '(' expression ')' block ELSE block

while_stmt      -> WHILE '(' expression ')' block

print_stmt      -> PRINT expression ';'

## 4. Assignment Statements
assign_stmt     -> IDENTIFIER '=' expression ';'

## 5. Expressions & Operator Precedence (Lowest to Highest Binding)
expression      -> logical_expr

logical_expr    -> logical_expr OR relational_expr
                 | relational_expr

relational_expr -> arithmetic_expr REL_OP arithmetic_expr
                 | arithmetic_expr

arithmetic_expr -> arithmetic_expr ADD_OP term
                 | term

term            -> term MUL_OP factor
                 | factor

factor          -> IDENTIFIER
                 | INT_LITERAL
                 | FLOAT_LITERAL
                 | BOOL_LITERAL
                 | '(' expression ')'
                 | NOT factor
                 | '-' factor

## 6. Terminal Lexical Categories
* **Keywords:** `int`, `float`, `bool`, `if`, `else`, `while`, `print`, `true`, `false`
* **Identifiers (`IDENTIFIER`):** `[a-zA-Z_][a-zA-Z0-9_]*`
* **Integer Literals (`INT_LITERAL`):** `[0-9]+`
* **Float Literals (`FLOAT_LITERAL`):** `[0-9]+\.[0-9]+`
* **Boolean Literals (`BOOL_LITERAL`):** `true`, `false`
* **Additive Operators (`ADD_OP`):** `+`, `-`
* **Multiplicative Operators (`MUL_OP`):** `*`, `/`, `%`
* **Relational Operators (`REL_OP`):** `<`, `>`, `<=`, `>=`, `==`, `!=`
* **Logical Operators (`OR`, `AND`, `NOT`):** `||`, `&&`, `!`