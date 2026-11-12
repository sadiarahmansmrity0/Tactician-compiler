%option noyywrap nounput noinput nodefault never-interactive

%{
#include "lexer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token scanner_token;
static int scanner_token_initialized = 0;
static int current_line = 1;
static int current_column = 1;
static SourceLocation match_location = {1, 1};
static SourceLocation comment_location = {1, 1};

static void lexer_mark_match(const char *text, int length);
static void lexer_emit(TokenKind kind, const char *text, int length);
static void lexer_emit_error(const char *message, const char *text, int length);
static void lexer_emit_invalid_character(unsigned char character);

#define YY_USER_ACTION lexer_mark_match(yytext, yyleng);

%}

%x BLOCK_COMMENT

DIGIT       [0-9]
LETTER      [A-Za-z_]
IDENTIFIER  {LETTER}[A-Za-z0-9_]*
INTEGER     {DIGIT}+
FLOATING    {DIGIT}+\.{DIGIT}+

%%

"int"                   { lexer_emit(TOKEN_INT, yytext, yyleng); return TOKEN_INT; }
"float"                 { lexer_emit(TOKEN_FLOAT, yytext, yyleng); return TOKEN_FLOAT; }
"bool"                  { lexer_emit(TOKEN_BOOL, yytext, yyleng); return TOKEN_BOOL; }
"if"                    { lexer_emit(TOKEN_IF, yytext, yyleng); return TOKEN_IF; }
"else"                  { lexer_emit(TOKEN_ELSE, yytext, yyleng); return TOKEN_ELSE; }
"while"                 { lexer_emit(TOKEN_WHILE, yytext, yyleng); return TOKEN_WHILE; }
"print"                 { lexer_emit(TOKEN_PRINT, yytext, yyleng); return TOKEN_PRINT; }
"true"                  { lexer_emit(TOKEN_TRUE, yytext, yyleng); return TOKEN_TRUE; }
"false"                 { lexer_emit(TOKEN_FALSE, yytext, yyleng); return TOKEN_FALSE; }

{FLOATING}               { lexer_emit(TOKEN_FLOAT_LITERAL, yytext, yyleng); return TOKEN_FLOAT_LITERAL; }
{INTEGER}                { lexer_emit(TOKEN_INT_LITERAL, yytext, yyleng); return TOKEN_INT_LITERAL; }
{IDENTIFIER}             { lexer_emit(TOKEN_IDENTIFIER, yytext, yyleng); return TOKEN_IDENTIFIER; }

"=="                    { lexer_emit(TOKEN_EQ, yytext, yyleng); return TOKEN_EQ; }
"!="                    { lexer_emit(TOKEN_NE, yytext, yyleng); return TOKEN_NE; }
"<="                    { lexer_emit(TOKEN_LE, yytext, yyleng); return TOKEN_LE; }
">="                    { lexer_emit(TOKEN_GE, yytext, yyleng); return TOKEN_GE; }
"&&"                    { lexer_emit(TOKEN_AND, yytext, yyleng); return TOKEN_AND; }
"||"                    { lexer_emit(TOKEN_OR, yytext, yyleng); return TOKEN_OR; }

"="                     { lexer_emit(TOKEN_ASSIGN, yytext, yyleng); return TOKEN_ASSIGN; }
"+"                     { lexer_emit(TOKEN_PLUS, yytext, yyleng); return TOKEN_PLUS; }
"-"                     { lexer_emit(TOKEN_MINUS, yytext, yyleng); return TOKEN_MINUS; }
"*"                     { lexer_emit(TOKEN_STAR, yytext, yyleng); return TOKEN_STAR; }
"/"                     { lexer_emit(TOKEN_SLASH, yytext, yyleng); return TOKEN_SLASH; }
"%"                     { lexer_emit(TOKEN_PERCENT, yytext, yyleng); return TOKEN_PERCENT; }
"<"                     { lexer_emit(TOKEN_LT, yytext, yyleng); return TOKEN_LT; }
">"                     { lexer_emit(TOKEN_GT, yytext, yyleng); return TOKEN_GT; }
"!"                     { lexer_emit(TOKEN_NOT, yytext, yyleng); return TOKEN_NOT; }

"{"                     { lexer_emit(TOKEN_LBRACE, yytext, yyleng); return TOKEN_LBRACE; }
"}"                     { lexer_emit(TOKEN_RBRACE, yytext, yyleng); return TOKEN_RBRACE; }
"("                     { lexer_emit(TOKEN_LPAREN, yytext, yyleng); return TOKEN_LPAREN; }
")"                     { lexer_emit(TOKEN_RPAREN, yytext, yyleng); return TOKEN_RPAREN; }
";"                     { lexer_emit(TOKEN_SEMICOLON, yytext, yyleng); return TOKEN_SEMICOLON; }

"//"[^\r\n]*           { /* Discard the comment; YY_USER_ACTION tracks its position. */ }
"/*"                    { comment_location = match_location; BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>\r\n     { /* Count Windows CRLF as one newline. */ }
<BLOCK_COMMENT>(.|\n)   { /* Discard comment contents; YY_USER_ACTION tracks newlines. */ }
<BLOCK_COMMENT><<EOF>>  {
                            char message[128];
                            snprintf(message, sizeof(message),
                                     "unterminated block comment starting at line %d, column %d",
                                     comment_location.line, comment_location.column);
                            BEGIN(INITIAL);
                            lexer_emit_error(message, "", 0);
                            scanner_token.location = comment_location;
                            return TOKEN_ERROR;
                        }

[ \t\f\v\r\n]+       { /* Discard whitespace; YY_USER_ACTION tracks its position. */ }

{DIGIT}+[eE][+-]?{DIGIT}+ {
                            lexer_emit_error("scientific notation is not supported", yytext, yyleng);
                            return TOKEN_ERROR;
                        }

.                       { lexer_emit_invalid_character((unsigned char)yytext[0]); return TOKEN_ERROR; }

<<EOF>>                 {
                            lexer_emit(TOKEN_EOF, "", 0);
                            return TOKEN_EOF;
                        }

%%

static void lexer_mark_match(const char *text, int length)
{
    int i;
    match_location.line = current_line;
    match_location.column = current_column;

    for (i = 0; i < length; ++i) {
        if (text[i] == '\r') {
            if (i + 1 < length && text[i + 1] == '\n') {
                ++i;
            }
            ++current_line;
            current_column = 1;
        } else if (text[i] == '\n') {
            ++current_line;
            current_column = 1;
        } else {
            ++current_column;
        }
    }
}

static void lexer_emit(TokenKind kind, const char *text, int length)
{
    char *end;
    if (!scanner_token_initialized) {
        token_init(&scanner_token);
        scanner_token_initialized = 1;
    }
    token_clear(&scanner_token);
    scanner_token.kind = kind;
    if (kind == TOKEN_EOF) {
        scanner_token.location.line = current_line;
        scanner_token.location.column = current_column;
    } else {
        scanner_token.location = match_location;
    }
    scanner_token.lexeme = token_copy_text(text, (size_t)length);
    if (scanner_token.lexeme == NULL) {
        scanner_token.kind = TOKEN_ERROR;
        scanner_token.message = token_copy_text("out of memory while storing token",
                                                strlen("out of memory while storing token"));
        return;
    }

    if (kind == TOKEN_INT_LITERAL) {
        errno = 0;
        scanner_token.integer_value = strtoll(scanner_token.lexeme, &end, 10);
        if (errno == ERANGE || *end != '\0') {
            scanner_token.kind = TOKEN_ERROR;
            scanner_token.message = token_copy_text("integer literal is outside the supported range",
                                                    strlen("integer literal is outside the supported range"));
        }
    } else if (kind == TOKEN_FLOAT_LITERAL) {
        errno = 0;
        scanner_token.float_value = strtod(scanner_token.lexeme, &end);
        if (errno == ERANGE || *end != '\0') {
            scanner_token.kind = TOKEN_ERROR;
            scanner_token.message = token_copy_text("floating-point literal is outside the supported range",
                                                    strlen("floating-point literal is outside the supported range"));
        }
    }
}

static void lexer_emit_error(const char *message, const char *text, int length)
{
    lexer_emit(TOKEN_ERROR, text, length);
    if (scanner_token.kind == TOKEN_ERROR) {
        free(scanner_token.message);
        scanner_token.message = token_copy_text(message, strlen(message));
    }
}

static void lexer_emit_invalid_character(unsigned char character)
{
    char message[96];
    if (character >= 32 && character <= 126) {
        snprintf(message, sizeof(message), "invalid character '%c'", character);
    } else {
        snprintf(message, sizeof(message), "invalid character byte 0x%02X", character);
    }
    lexer_emit_error(message, (const char *)&character, 1);
}

void lexer_set_input(FILE *input)
{
    if (!scanner_token_initialized) {
        token_init(&scanner_token);
        scanner_token_initialized = 1;
    }
    token_clear(&scanner_token);
    current_line = 1;
    current_column = 1;
    match_location.line = 1;
    match_location.column = 1;
    comment_location.line = 1;
    comment_location.column = 1;
    BEGIN(INITIAL);
    yyrestart(input);
}

TokenKind lexer_next(Token *token)
{
    int kind;
    if (token == NULL) {
        return TOKEN_ERROR;
    }
    token_clear(token);
    (void)yylex();
    kind = scanner_token.kind;
    *token = scanner_token;
    token_init(&scanner_token);
    return (TokenKind)kind;
}
