#ifndef TACTICIAN_LEXER_H
#define TACTICIAN_LEXER_H

#include <stdio.h>
#include "token.h"

/* The caller owns input. The lexer reads from it until EOF. */
void lexer_set_input(FILE *input);

/* Fills token and returns its kind. The caller must initialize token once. */
TokenKind lexer_next(Token *token);

#endif
