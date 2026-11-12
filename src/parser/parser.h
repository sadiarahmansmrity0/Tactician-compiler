#ifndef TACTICIAN_PARSER_H
#define TACTICIAN_PARSER_H

#include <stdio.h>
#include "../ast/ast.h"

/* Returns zero only when parsing succeeds without lexical or syntax errors. */
int parser_parse(FILE *input, AstProgram **result, FILE *diagnostics);

#endif
