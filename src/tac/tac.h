#ifndef TACTICIAN_TAC_H
#define TACTICIAN_TAC_H

#include <stdio.h>
#include "../ast/ast.h"

typedef enum TacStatus {
    TAC_STATUS_OK,
    TAC_STATUS_INVALID_ARGUMENT,
    TAC_STATUS_OUT_OF_MEMORY,
    TAC_STATUS_OUTPUT_ERROR,
    TAC_STATUS_UNSUPPORTED_AST
} TacStatus;

/* The caller must pass a program that has passed semantic analysis. */
TacStatus tac_generate(const AstProgram *program, FILE *output);
const char *tac_status_message(TacStatus status);

#endif
