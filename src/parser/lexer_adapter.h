#ifndef TACTICIAN_LEXER_ADAPTER_H
#define TACTICIAN_LEXER_ADAPTER_H

#include <stdio.h>

void parser_lexer_adapter_reset(FILE *input, FILE *diagnostics);
int parser_lexer_adapter_error_count(void);
const char *parser_lexer_adapter_current_lexeme(void);

#endif
