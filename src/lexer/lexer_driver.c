#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

static void print_quoted(const char *text)
{
    const unsigned char *cursor = (const unsigned char *)text;
    putchar('"');
    while (*cursor != '\0') {
        if (*cursor == '\\' || *cursor == '"') {
            putchar('\\');
            putchar(*cursor);
        } else if (*cursor == '\n') {
            fputs("\\n", stdout);
        } else if (*cursor == '\r') {
            fputs("\\r", stdout);
        } else if (*cursor == '\t') {
            fputs("\\t", stdout);
        } else if (*cursor < 32 || *cursor > 126) {
            printf("\\x%02X", *cursor);
        } else {
            putchar(*cursor);
        }
        ++cursor;
    }
    putchar('"');
}

int main(int argc, char **argv)
{
    FILE *input = stdin;
    Token token;
    int had_error = 0;
    TokenKind kind;

    if (argc > 2) {
        fprintf(stderr, "usage: %s [source-file]\n", argv[0]);
        return 2;
    }
    if (argc == 2) {
        input = fopen(argv[1], "rb");
        if (input == NULL) {
            perror(argv[1]);
            return 2;
        }
    }

    token_init(&token);
    lexer_set_input(input);

    do {
        kind = lexer_next(&token);
        if (kind == TOKEN_ERROR) {
            had_error = 1;
            printf("error %d:%d: %s", token.location.line,
                   token.location.column,
                   token.message != NULL ? token.message : "lexical error");
            if (token.lexeme != NULL && token.lexeme[0] != '\0') {
                fputs(" near ", stdout);
                print_quoted(token.lexeme);
            }
            putchar('\n');
        } else {
            printf("%d:%d %-14s", token.location.line, token.location.column,
                   token_kind_name(kind));
            if (token.lexeme != NULL && token.lexeme[0] != '\0') {
                putchar(' ');
                print_quoted(token.lexeme);
            }
            if (kind == TOKEN_INT_LITERAL) {
                printf(" value=%lld", token.integer_value);
            } else if (kind == TOKEN_FLOAT_LITERAL) {
                printf(" value=%.15g", token.float_value);
            }
            putchar('\n');
        }
    } while (kind != TOKEN_EOF);

    token_clear(&token);
    if (input != stdin) {
        fclose(input);
    }
    return had_error ? 1 : 0;
}
