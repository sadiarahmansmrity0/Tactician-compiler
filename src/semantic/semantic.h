#ifndef TACTICIAN_SEMANTIC_H
#define TACTICIAN_SEMANTIC_H

#include <stdio.h>
#include "../ast/ast.h"

typedef struct SemanticResult {
    int success;
    int error_count;
} SemanticResult;

/* Analyzes a parsed program in source order and writes diagnostics to the stream. */
SemanticResult semantic_analyze(const AstProgram *program, FILE *diagnostics);

#endif
