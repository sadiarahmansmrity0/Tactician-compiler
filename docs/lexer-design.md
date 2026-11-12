# Lexer Design

## Purpose

Lexical analysis reads source characters and groups them into tokens, such as keywords, identifiers, literals, operators, and delimiters. The lexer also skips whitespace/comments and reports characters that do not belong to the language. It is implemented with Flex and C.

## Token interface

`src/lexer/token.h` defines the shared `TokenKind` enum and `Token` structure. Each token carries its kind, original lexeme when applicable, one-based source line and column, and a converted `long long` or `double` value for numeric literals. Error tokens carry a diagnostic message.

The scanner API in `src/lexer/lexer.h` is:

```c
void lexer_set_input(FILE *input);
TokenKind lexer_next(Token *token);
```

The caller owns the input stream and initializes a `Token` with `token_init` before its first call to `lexer_next`. Each returned token is owned by that `Token`; call `token_clear` before it goes out of scope. A future Bison interface can map `TokenKind` values to Bison token constants and transfer lexemes/locations without changing the scanner rules.

## Token categories and patterns

The approved terminals are represented in `token.h`: seven keywords (`int`, `float`, `bool`, `if`, `else`, `while`, `print`), Boolean literals (`true`, `false`), identifiers, integer and floating-point literals, assignment, arithmetic/relational/equality/logical operators, delimiters, lexical errors, and EOF.

The principal Flex regular expressions in `src/lexer/lexer.l` are:

```text
DIGIT       [0-9]
LETTER      [A-Za-z_]
IDENTIFIER  {LETTER}[A-Za-z0-9_]*
INTEGER     {DIGIT}+
FLOATING    {DIGIT}+\.{DIGIT}+
```

The floating rule appears before the integer rule. The lexer also has explicit rules for scientific notation so a form such as `1e3` produces a clear lexical error rather than appearing to be an integer followed by an identifier. This is consistent with the approved language decision that scientific notation is unsupported. Numeric values are converted with `strtoll` and `strtod`; values outside the supported C numeric range are reported as lexical errors.

## Keywords and longest match

Keyword spellings have explicit rules before the generic identifier rule. Flex first chooses the rule matching the most input characters; if two rules match the same number, the earlier rule wins. Thus `int` matches the keyword rule, while `integer` matches as one complete identifier because that rule matches more characters than the `int` keyword prefix.

The two-character operators (`==`, `!=`, `<=`, `>=`, `&&`, `||`) have rules before their one-character prefixes. Flex's longest-match behavior also selects the longer form when both can match. Consequently `=` and `==`, `!` and `!=`, and `<` and `<=` produce distinct tokens. A lone `&` or `|` is not valid and produces an error; `&&` and `||` are recognized as logical operators.

## Comments and whitespace

The lexer discards `//` through the end of the line. On seeing `/*`, it enters a Flex start condition that consumes characters until `*/`. Both forms are discarded rather than returned as tokens. If EOF occurs while inside a block comment, one error reports the opening delimiter's line and column; the next call returns EOF safely.

Spaces, tabs, form feeds, vertical tabs, and line endings are discarded. Line endings LF, CR, and CRLF update the line count; CRLF counts as one newline, including inside block comments.

## Locations and errors

The location tracker runs on every matched lexeme, including skipped spaces and comments. It records the token's starting location and advances a one-based line/column position. Tabs count as one column. Because the language's identifiers and operators are ASCII, columns count input bytes; a non-ASCII character is reported by its invalid byte value.

Any unmatched character produces a `TOKEN_ERROR` with a message and location. Scanning continues so later tokens and errors can also be shown. The standalone driver prints tokens and errors with locations and exits with status 1 if any lexical error occurred. EOF reports the current source position, including after a final newline.

## Build and test

With GCC, Flex, and Make available on `PATH`:

```sh
make lexer
make test
```

On Windows with the checked MinGW/MSYS tools, the executable is `build/tactician-lexer.exe`. Run it on a file with:

```sh
build/tactician-lexer.exe tests/cases/all_tokens.tac
```

The standalone driver is `src/lexer/lexer_driver.c`; the scanner specification is `src/lexer/lexer.l`; token definitions and the scanner API are in `src/lexer/token.h` and `src/lexer/lexer.h`. Flex generates `build/lexer.c` during the build; it is generated output and should not be edited by hand.

`make test` rebuilds the lexer and compares its output with checked-in expected output for the token inventory, operator-prefix cases, comment forms, locations (including CRLF), invalid characters, unsupported scientific notation, and an unterminated block comment. Tests also check the expected exit status for error inputs.
