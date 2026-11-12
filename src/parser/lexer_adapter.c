#include "lexer_adapter.h"

#include "parser.tab.h"
#include "../lexer/lexer.h"

#include <stdlib.h>
#include <string.h>

static Token adapter_token;
static int adapter_initialized = 0;
static int lexical_errors = 0;
static FILE *diagnostic_stream = NULL;
static char *current_lexeme = NULL;

static void remember_lexeme(const char *text)
{
    size_t length = text != NULL ? strlen(text) : 0;
    char *copy = token_copy_text(text != NULL ? text : "", length);
    free(current_lexeme);
    current_lexeme = copy;
}

void parser_lexer_adapter_reset(FILE *input, FILE *diagnostics)
{
    if (!adapter_initialized) {
        token_init(&adapter_token);
        adapter_initialized = 1;
    }
    token_clear(&adapter_token);
    free(current_lexeme);
    current_lexeme = NULL;
    lexical_errors = 0;
    diagnostic_stream = diagnostics != NULL ? diagnostics : stderr;
    lexer_set_input(input);
}

int parser_lexer_adapter_error_count(void)
{
    return lexical_errors;
}

const char *parser_lexer_adapter_current_lexeme(void)
{
    return current_lexeme;
}

static int map_token(TokenKind kind)
{
    switch (kind) {
    case TOKEN_EOF: return 0;
    case TOKEN_INT: return T_INT;
    case TOKEN_FLOAT: return T_FLOAT;
    case TOKEN_BOOL: return T_BOOL;
    case TOKEN_IF: return T_IF;
    case TOKEN_ELSE: return T_ELSE;
    case TOKEN_WHILE: return T_WHILE;
    case TOKEN_PRINT: return T_PRINT;
    case TOKEN_TRUE: return T_TRUE;
    case TOKEN_FALSE: return T_FALSE;
    case TOKEN_IDENTIFIER: return T_IDENTIFIER;
    case TOKEN_INT_LITERAL: return T_INT_LITERAL;
    case TOKEN_FLOAT_LITERAL: return T_FLOAT_LITERAL;
    case TOKEN_ASSIGN: return T_ASSIGN;
    case TOKEN_PLUS: return T_PLUS;
    case TOKEN_MINUS: return T_MINUS;
    case TOKEN_STAR: return T_STAR;
    case TOKEN_SLASH: return T_SLASH;
    case TOKEN_PERCENT: return T_PERCENT;
    case TOKEN_LT: return T_LT;
    case TOKEN_GT: return T_GT;
    case TOKEN_LE: return T_LE;
    case TOKEN_GE: return T_GE;
    case TOKEN_EQ: return T_EQ;
    case TOKEN_NE: return T_NE;
    case TOKEN_AND: return T_AND;
    case TOKEN_OR: return T_OR;
    case TOKEN_NOT: return T_NOT;
    case TOKEN_LBRACE: return T_LBRACE;
    case TOKEN_RBRACE: return T_RBRACE;
    case TOKEN_LPAREN: return T_LPAREN;
    case TOKEN_RPAREN: return T_RPAREN;
    case TOKEN_SEMICOLON: return T_SEMICOLON;
    case TOKEN_ERROR:
        break;
    }
    return 0;
}

int yylex(void)
{
    TokenKind kind;

    do {
        kind = lexer_next(&adapter_token);
        if (kind == TOKEN_ERROR) {
            ++lexical_errors;
            fprintf(diagnostic_stream, "%d:%d: lexical error: %s",
                    adapter_token.location.line, adapter_token.location.column,
                    adapter_token.message != NULL ? adapter_token.message : "invalid token");
            if (adapter_token.lexeme != NULL && adapter_token.lexeme[0] != '\0') {
                fprintf(diagnostic_stream, " near '%s'", adapter_token.lexeme);
            }
            fputc('\n', diagnostic_stream);
        }
    } while (kind == TOKEN_ERROR);

    remember_lexeme(adapter_token.lexeme);
    yylloc.first_line = adapter_token.location.line;
    yylloc.first_column = adapter_token.location.column;
    yylloc.last_line = adapter_token.location.line;
    yylloc.last_column = adapter_token.location.column;

    switch (kind) {
    case TOKEN_IDENTIFIER:
        yylval.text = adapter_token.lexeme;
        adapter_token.lexeme = NULL;
        break;
    case TOKEN_INT_LITERAL:
        yylval.integer_value = adapter_token.integer_value;
        break;
    case TOKEN_FLOAT_LITERAL:
        yylval.float_value = adapter_token.float_value;
        break;
    default:
        break;
    }

    token_clear(&adapter_token);
    return map_token(kind);
}
