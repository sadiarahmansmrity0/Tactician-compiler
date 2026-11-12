# Tactician Context-Free Grammar

This document records the approved grammar and project-defined language rules for Tactician. The Compiler Construction Lab Manual remains authoritative for required language features. Rules that the manual leaves unspecified are identified below as project-defined decisions.

## 1. Formal CFG

The grammar is written in BNF-style notation. Uppercase names denote lexer tokens. Quoted punctuation denotes literal tokens. `ε` denotes the empty sequence.

```text
program          -> statement_list

statement_list   -> statement statement_list
                  | ε

statement        -> declaration
                  | assignment
                  | if_statement
                  | while_statement
                  | print_statement
                  | block

declaration      -> type IDENTIFIER ';'

type             -> INT
                  | FLOAT
                  | BOOL

assignment       -> IDENTIFIER '=' expression ';'

if_statement     -> IF '(' expression ')' block
                  | IF '(' expression ')' block ELSE block

while_statement  -> WHILE '(' expression ')' block

print_statement  -> PRINT expression ';'

block            -> '{' statement_list '}'

expression       -> logical_or

logical_or       -> logical_or OR logical_and
                  | logical_and

logical_and      -> logical_and AND equality
                  | equality

equality         -> relational
                  | relational EQ relational
                  | relational NE relational

relational       -> additive
                  | additive LT additive
                  | additive GT additive
                  | additive LE additive
                  | additive GE additive

additive         -> additive PLUS multiplicative
                  | additive MINUS multiplicative
                  | multiplicative

multiplicative   -> multiplicative STAR unary
                  | multiplicative SLASH unary
                  | multiplicative PERCENT unary
                  | unary

unary            -> NOT unary
                  | primary

primary          -> IDENTIFIER
                  | INT_LITERAL
                  | FLOAT_LITERAL
                  | TRUE
                  | FALSE
                  | '(' expression ')'
```

The `relational` and `equality` rules each allow at most one comparison operator. Their operands cannot themselves be another relational or equality expression, so `a < b < c` and `a == b == c` are rejected. A comparison on each side of a logical operator remains valid, as in `a < b && b < c`.

## 2. Non-terminals

`program`, `statement_list`, `statement`, `declaration`, `type`, `assignment`, `if_statement`, `while_statement`, `print_statement`, `block`, `expression`, `logical_or`, `logical_and`, `equality`, `relational`, `additive`, `multiplicative`, `unary`, `primary`.

## 3. Terminals

| Category | Tokens and source spellings |
|---|---|
| Types/keywords | `INT` (`int`), `FLOAT` (`float`), `BOOL` (`bool`), `IF` (`if`), `ELSE` (`else`), `WHILE` (`while`), `PRINT` (`print`) |
| Boolean literals | `TRUE` (`true`), `FALSE` (`false`) |
| Names/literals | `IDENTIFIER`, `INT_LITERAL`, `FLOAT_LITERAL` |
| Assignment | `'='` |
| Arithmetic | `PLUS` (`+`), `MINUS` (`-`), `STAR` (`*`), `SLASH` (`/`), `PERCENT` (`%`) |
| Relational | `LT` (`<`), `GT` (`>`), `LE` (`<=`), `GE` (`>=`) |
| Equality | `EQ` (`==`), `NE` (`!=`) |
| Logical | `AND` (`&&`), `OR` (`||`), `NOT` (`!`) |
| Delimiters | `'{'`, `'}'`, `'('`, `')'`, `';'` |

The lexer discards whitespace and both `//` single-line and `/* ... */` block comments. They are not parser terminals. Identifier spelling is `[A-Za-z_][A-Za-z0-9_]*`; integer literals are `[0-9]+`; floating-point literals are `[0-9]+\.[0-9]+`. Scientific notation is not supported. The manual requires comments to be discarded; the two comment forms and exact literal patterns are project-defined decisions.

## 4. Precedence and Associativity

From tightest binding to loosest:

| Level | Operators | Associativity / chaining |
|---:|---|---|
| 1 | `!` | Right-associative through recursive unary expressions |
| 2 | `*`, `/`, `%` | Left-associative |
| 3 | `+`, `-` | Left-associative |
| 4 | `<`, `>`, `<=`, `>=` | Chaining rejected |
| 5 | `==`, `!=` | Chaining rejected |
| 6 | `&&` | Left-associative |
| 7 | `||` | Left-associative |

Examples of the resulting grouping:

- `a - b - c` groups as `(a - b) - c`.
- `a + b * c` groups as `a + (b * c)`.
- `!!flag && x < y` groups as `((!(!flag)) && (x < y))`.
- `a < b && b < c || ready` groups as `((a < b) && (b < c)) || ready`.
- `a == b || c == d && ready` groups as `(a == b) || ((c == d) && ready)`.

The manual specifies the operators but not their precedence or associativity. This table is the approved project-defined convention.

## 5. Valid Examples

These programs are syntactically valid; semantic validity depends on declarations and types.

```text
// Both comment forms are discarded.
int count;
float ratio;
bool ready;

count = 2 + 3 * 4;
ratio = count / 2.0;
ready = !(count < 20) || false;

if (count < 20 && ratio != 0.0) {
    print count;
    {
        bool nested;
        nested = true;
    }
} else {
    print ratio;
}

while (count > 0) {
    count = count - 1;
}

/* Empty blocks and an empty program are permitted. */
{}
```

The empty program is the source containing no statements. An empty block is `{}`.

## 6. Invalid Examples

Each item is an invalid complete program or statement under this grammar:

```text
int value = 5;       // invalid: declaration initialization is not supported
value = -5;          // invalid: unary minus is excluded
bool result;
result = a < b < c;  // invalid: chained relational comparisons are rejected
result = a == b == c; // invalid: chained equality comparisons are rejected
value = 1e3;         // invalid: scientific notation is not supported
int a, b;            // invalid: one identifier per declaration only
if (ready) print 1;  // invalid: if bodies must be blocks
```

These examples illustrate syntax. Type errors such as using a numeric condition are handled by semantic analysis, not by the CFG.

## 7. Semantic Compatibility Rules

The manual requires semantic analysis to detect type mismatches, invalid assignments, and invalid expressions. It does not specify the complete compatibility rules. The following table records the approved **project-defined** rules.

| Construct | Accepted operand/source types | Result or target behavior |
|---|---|---|
| `+`, `-`, `*` | `int/int`, `int/float`, `float/int`, `float/float` | `int` for `int/int`; otherwise `float` after numeric promotion |
| `/` | Same numeric combinations as above | `int` for `int/int`; otherwise `float` |
| `%` | `int/int` only | `int` |
| `<`, `>`, `<=`, `>=` | Any pair of numeric types (`int` and/or `float`) | `bool`; mixed operands promote `int` to `float` |
| `==`, `!=` | Numeric pairs (`int` and/or `float`), or `bool/bool` | `bool`; mixed numeric operands promote `int` to `float` |
| `&&`, `||` | `bool/bool` only | `bool` |
| `!` | `bool` only | `bool` |
| Assignment to `int` | `int` expression | Accepted; `float` expression is rejected (no narrowing) |
| Assignment to `float` | `float` or `int` expression | Accepted; `int` widens to `float` |
| Assignment to `bool` | `bool` expression | Accepted; numeric expressions are rejected |
| `if` / `while` condition | `bool` expression only | Numeric conditions are rejected |

Integer division produces an integer according to the approved project decision. The manual requires TAC for arithmetic, relational and logical expressions, but does not define these typing, promotion, or division-result rules.

## 8. Language Decisions and Authority

The Compiler Construction Lab Manual remains authoritative for required features: the three data types, listed keywords and literals, arithmetic/relational/logical operator set, declarations, assignments, blocks, `if`, `if-else`, `while`, and `print`. This document does not add arrays, functions, loops beyond `while`, declaration initialization, or unary minus.

The following approved project decisions fill details the manual leaves open:

- Discard both `//` line comments and `/* ... */` block comments.
- Use the literal formats and identifier pattern documented above; do not accept scientific notation.
- Use the precedence and associativity table in Section 4.
- Reject chained relational and equality comparisons.
- Permit unary logical NOT; exclude unary minus.
- Use the compatibility and promotion rules in Section 7, including integer division producing `int`.
- Permit one variable per declaration, with no declaration initialization.
- Permit empty programs and empty blocks; blocks may nest.
