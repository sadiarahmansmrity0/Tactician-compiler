#ifndef TACTICIAN_TOKEN_H
#define TACTICIAN_TOKEN_H

#include <stddef.h>
#include "../common/source_location.h"

typedef enum TokenKind {
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_BOOL,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_PRINT,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SEMICOLON
} TokenKind;

typedef struct Token {
    TokenKind kind;
    char *lexeme;
    char *message;
    long long integer_value;
    double float_value;
    SourceLocation location;
} Token;

const char *token_kind_name(TokenKind kind);
void token_init(Token *token);
void token_clear(Token *token);
char *token_copy_text(const char *text, size_t length);

#endif
