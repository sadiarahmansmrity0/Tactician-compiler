# Project Report

## 9. Lexer Design

The lexer is implemented using Flex and C. It scans the input stream and returns tokens through the shared `Token` structure, which records token kind, source lexeme, one-based line and column, and converted numeric values where relevant. The scanner API is `lexer_set_input(FILE *)` followed by repeated calls to `lexer_next(Token *)` until `TOKEN_EOF`.

Explicit rules recognize the approved keywords and Boolean literals before the generic identifier rule. Flex's longest-match behavior distinguishes multi-character operators such as `==`, `!=`, `<=`, `>=`, `&&`, and `||` from their one-character forms. Identifiers use `[A-Za-z_][A-Za-z0-9_]*`; integers use `[0-9]+`; floats use `[0-9]+\.[0-9]+`. Scientific notation is reported as unsupported.

Whitespace, `//` comments, and `/* ... */` comments are discarded. A Flex start condition reads block comments and reports an unterminated comment at its opening location. The location tracker counts LF, CR, and CRLF correctly and retains token start positions. Invalid characters produce location-bearing lexical errors; scanning continues, and the standalone driver returns a nonzero status if any error was found.

The lexer was built from a clean build directory with GCC and Flex. Running `make test` executed eight focused cases; all eight passed. The test cases are under `tests/cases/`, with their expected output under `tests/expected/`.
