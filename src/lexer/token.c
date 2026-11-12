#include "token.h"

#include <stdlib.h>

const char *token_kind_name(TokenKind kind)
{
    static const char *names[] = {
        "EOF", "ERROR", "INT", "FLOAT", "BOOL", "IF", "ELSE",
        "WHILE", "PRINT", "TRUE", "FALSE", "IDENTIFIER",
        "INT_LITERAL", "FLOAT_LITERAL", "ASSIGN", "PLUS", "MINUS",
        "STAR", "SLASH", "PERCENT", "LT", "GT", "LE", "GE", "EQ",
        "NE", "AND", "OR", "NOT", "LBRACE", "RBRACE", "LPAREN",
        "RPAREN", "SEMICOLON"
    };

    if ((size_t)kind >= sizeof(names) / sizeof(names[0])) {
        return "UNKNOWN";
    }
    return names[kind];
}

void token_init(Token *token)
{
    token->kind = TOKEN_EOF;
    token->lexeme = NULL;
    token->message = NULL;
    token->integer_value = 0;
    token->float_value = 0.0;
    token->location.line = 1;
    token->location.column = 1;
}

void token_clear(Token *token)
{
    free(token->lexeme);
    free(token->message);
    token_init(token);
}

char *token_copy_text(const char *text, size_t length)
{
    char *copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < length; ++i) {
        copy[i] = text[i];
    }
    copy[length] = '\0';
    return copy;
}
