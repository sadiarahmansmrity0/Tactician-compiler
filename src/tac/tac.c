#include "tac.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TacContext {
    FILE *output;
    unsigned int next_temporary;
    unsigned int next_label;
    TacStatus status;
} TacContext;

static int emit(TacContext *context, const char *format, ...)
{
    va_list arguments;
    int result;
    if (context->status != TAC_STATUS_OK) return 0;
    va_start(arguments, format);
    result = vfprintf(context->output, format, arguments);
    va_end(arguments);
    if (result < 0 || ferror(context->output)) {
        context->status = TAC_STATUS_OUTPUT_ERROR;
        return 0;
    }
    return 1;
}

static char *copy_text(const char *text)
{
    size_t length;
    char *copy;
    if (text == NULL) return NULL;
    length = strlen(text);
    copy = (char *)malloc(length + 1);
    if (copy != NULL) memcpy(copy, text, length + 1);
    return copy;
}

static void format_integer(char *buffer, size_t capacity, long long value)
{
    char reversed[32];
    size_t count = 0;
    size_t output = 0;
    unsigned long long magnitude;
    int negative = value < 0;

    /* Avoid compiler-specific printf length modifiers on older MinGW. */
    magnitude = negative ? (unsigned long long)(-(value + 1)) + 1ULL
                         : (unsigned long long)value;
    do {
        reversed[count++] = (char)('0' + (magnitude % 10ULL));
        magnitude /= 10ULL;
    } while (magnitude != 0ULL && count < sizeof(reversed));
    if (negative && output + 1 < capacity) buffer[output++] = '-';
    while (count > 0 && output + 1 < capacity)
        buffer[output++] = reversed[--count];
    buffer[output] = '\0';
}

static char *new_temporary(TacContext *context)
{
    char buffer[40];
    if (context->next_temporary == 0) {
        context->status = TAC_STATUS_OUT_OF_MEMORY;
        return NULL;
    }
    snprintf(buffer, sizeof(buffer), "t%u", context->next_temporary++);
    {
        char *name = copy_text(buffer);
        if (name == NULL) context->status = TAC_STATUS_OUT_OF_MEMORY;
        return name;
    }
}

static char *new_label(TacContext *context)
{
    char buffer[40];
    if (context->next_label == 0) {
        context->status = TAC_STATUS_OUT_OF_MEMORY;
        return NULL;
    }
    snprintf(buffer, sizeof(buffer), "L%u", context->next_label++);
    {
        char *name = copy_text(buffer);
        if (name == NULL) context->status = TAC_STATUS_OUT_OF_MEMORY;
        return name;
    }
}

static int is_comparison(AstBinaryOp op)
{
    return op == AST_OP_LESS || op == AST_OP_GREATER ||
           op == AST_OP_LESS_EQUAL || op == AST_OP_GREATER_EQUAL ||
           op == AST_OP_EQUAL || op == AST_OP_NOT_EQUAL;
}

static int emit_expression(TacContext *context, const AstExpr *expression,
                           char **result);
static int emit_condition(TacContext *context, const AstExpr *expression,
                          const char *true_label, const char *false_label);
static int emit_statement(TacContext *context, const AstStmt *statement);

static int emit_boolean_value(TacContext *context, const AstExpr *expression,
                              char **result)
{
    char *temporary = new_temporary(context);
    char *true_label = new_label(context);
    char *done_label = new_label(context);
    int success = 0;

    if (temporary == NULL || true_label == NULL || done_label == NULL) goto cleanup;
    if (!emit(context, "%s = false\n", temporary)) goto cleanup;
    if (!emit_condition(context, expression, true_label, done_label)) goto cleanup;
    if (!emit(context, "%s:\n", true_label)) goto cleanup;
    if (!emit(context, "%s = true\n", temporary)) goto cleanup;
    if (!emit(context, "%s:\n", done_label)) goto cleanup;
    *result = temporary;
    temporary = NULL;
    success = 1;

cleanup:
    free(temporary);
    free(true_label);
    free(done_label);
    return success;
}

static int emit_expression(TacContext *context, const AstExpr *expression,
                           char **result)
{
    char buffer[96];
    char *left = NULL;
    char *right = NULL;
    char *temporary = NULL;
    AstBinaryOp op;
    if (result == NULL || expression == NULL) {
        context->status = TAC_STATUS_UNSUPPORTED_AST;
        return 0;
    }
    *result = NULL;

    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        *result = copy_text(expression->value.identifier);
        if (*result == NULL) context->status = TAC_STATUS_OUT_OF_MEMORY;
        return *result != NULL;

    case AST_EXPR_INTEGER:
        format_integer(buffer, sizeof(buffer), expression->value.integer);
        *result = copy_text(buffer);
        if (*result == NULL) context->status = TAC_STATUS_OUT_OF_MEMORY;
        return *result != NULL;

    case AST_EXPR_FLOAT:
        snprintf(buffer, sizeof(buffer), "%.15g", expression->value.floating);
        *result = copy_text(buffer);
        if (*result == NULL) context->status = TAC_STATUS_OUT_OF_MEMORY;
        return *result != NULL;

    case AST_EXPR_BOOLEAN:
        *result = copy_text(expression->value.boolean ? "true" : "false");
        if (*result == NULL) context->status = TAC_STATUS_OUT_OF_MEMORY;
        return *result != NULL;

    case AST_EXPR_UNARY:
        if (expression->value.unary.op != AST_OP_NOT) {
            context->status = TAC_STATUS_UNSUPPORTED_AST;
            return 0;
        }
        if (!emit_expression(context, expression->value.unary.operand, &left))
            return 0;
        temporary = new_temporary(context);
        if (temporary == NULL) {
            free(left);
            return 0;
        }
        if (!emit(context, "%s = !%s\n", temporary, left)) {
            free(left);
            free(temporary);
            return 0;
        }
        free(left);
        *result = temporary;
        return 1;

    case AST_EXPR_BINARY:
        op = expression->value.binary.op;
        if (op == AST_OP_AND || op == AST_OP_OR || is_comparison(op))
            return emit_boolean_value(context, expression, result);
        if (op < AST_OP_ADD || op > AST_OP_MODULO) {
            context->status = TAC_STATUS_UNSUPPORTED_AST;
            return 0;
        }
        if (!emit_expression(context, expression->value.binary.left, &left))
            return 0;
        if (!emit_expression(context, expression->value.binary.right, &right)) {
            free(left);
            return 0;
        }
        temporary = new_temporary(context);
        if (temporary == NULL) {
            free(left);
            free(right);
            return 0;
        }
        if (!emit(context, "%s = %s %s %s\n", temporary, left,
                  ast_binary_op_name(op), right)) {
            free(left);
            free(right);
            free(temporary);
            return 0;
        }
        free(left);
        free(right);
        *result = temporary;
        return 1;
    }

    context->status = TAC_STATUS_UNSUPPORTED_AST;
    return 0;
}

static int emit_condition(TacContext *context, const AstExpr *expression,
                          const char *true_label, const char *false_label)
{
    char *left = NULL;
    char *right = NULL;
    AstBinaryOp op;
    int success;
    if (expression == NULL) {
        context->status = TAC_STATUS_UNSUPPORTED_AST;
        return 0;
    }

    if (expression->kind == AST_EXPR_BOOLEAN) {
        return emit(context, "goto %s\n",
                    expression->value.boolean ? true_label : false_label);
    }
    if (expression->kind == AST_EXPR_UNARY) {
        if (expression->value.unary.op != AST_OP_NOT) {
            context->status = TAC_STATUS_UNSUPPORTED_AST;
            return 0;
        }
        return emit_condition(context, expression->value.unary.operand,
                              false_label, true_label);
    }
    if (expression->kind == AST_EXPR_BINARY) {
        op = expression->value.binary.op;
        if (op == AST_OP_AND) {
            char *right_label = new_label(context);
            if (right_label == NULL) return 0;
            success = emit_condition(context, expression->value.binary.left,
                                     right_label, false_label);
            if (success) success = emit(context, "%s:\n", right_label);
            if (success)
                success = emit_condition(context, expression->value.binary.right,
                                         true_label, false_label);
            free(right_label);
            return success;
        }
        if (op == AST_OP_OR) {
            char *right_label = new_label(context);
            if (right_label == NULL) return 0;
            success = emit_condition(context, expression->value.binary.left,
                                     true_label, right_label);
            if (success) success = emit(context, "%s:\n", right_label);
            if (success)
                success = emit_condition(context, expression->value.binary.right,
                                         true_label, false_label);
            free(right_label);
            return success;
        }
        if (is_comparison(op)) {
            if (!emit_expression(context, expression->value.binary.left, &left))
                return 0;
            if (!emit_expression(context, expression->value.binary.right, &right)) {
                free(left);
                return 0;
            }
            success = emit(context, "if %s %s %s goto %s\n", left,
                           ast_binary_op_name(op), right, true_label);
            if (success) success = emit(context, "goto %s\n", false_label);
            free(left);
            free(right);
            return success;
        }
    }

    if (!emit_expression(context, expression, &left)) return 0;
    success = emit(context, "if %s goto %s\n", left, true_label);
    if (success) success = emit(context, "goto %s\n", false_label);
    free(left);
    return success;
}

static int emit_statement_list(TacContext *context, const AstStmtList *list)
{
    for (; list != NULL; list = list->next) {
        if (!emit_statement(context, list->statement)) return 0;
    }
    return 1;
}

static int emit_if(TacContext *context, const AstStmt *statement)
{
    char *then_label = new_label(context);
    char *else_label = NULL;
    char *end_label = NULL;
    int has_else = statement->kind == AST_STMT_IF_ELSE;
    int success = 0;
    if (then_label == NULL) goto cleanup;
    if (has_else) {
        else_label = new_label(context);
        end_label = new_label(context);
    } else {
        end_label = new_label(context);
    }
    if (end_label == NULL || (has_else && else_label == NULL)) goto cleanup;

    if (!emit_condition(context, statement->value.conditional.condition,
                        then_label, has_else ? else_label : end_label)) goto cleanup;
    if (!emit(context, "%s:\n", then_label)) goto cleanup;
    if (!emit_statement(context, statement->value.conditional.then_branch)) goto cleanup;
    if (has_else) {
        if (!emit(context, "goto %s\n", end_label)) goto cleanup;
        if (!emit(context, "%s:\n", else_label)) goto cleanup;
        if (!emit_statement(context, statement->value.conditional.else_branch)) goto cleanup;
    }
    if (!emit(context, "%s:\n", end_label)) goto cleanup;
    success = 1;

cleanup:
    free(then_label);
    free(else_label);
    free(end_label);
    return success;
}

static int emit_while(TacContext *context, const AstStmt *statement)
{
    char *test_label = new_label(context);
    char *body_label = new_label(context);
    char *end_label = new_label(context);
    int success = 0;
    if (test_label == NULL || body_label == NULL || end_label == NULL) goto cleanup;
    if (!emit(context, "%s:\n", test_label)) goto cleanup;
    if (!emit_condition(context, statement->value.loop.condition,
                        body_label, end_label)) goto cleanup;
    if (!emit(context, "%s:\n", body_label)) goto cleanup;
    if (!emit_statement(context, statement->value.loop.body)) goto cleanup;
    if (!emit(context, "goto %s\n", test_label)) goto cleanup;
    if (!emit(context, "%s:\n", end_label)) goto cleanup;
    success = 1;

cleanup:
    free(test_label);
    free(body_label);
    free(end_label);
    return success;
}

static int emit_statement(TacContext *context, const AstStmt *statement)
{
    char *value = NULL;
    int success;
    if (statement == NULL) {
        context->status = TAC_STATUS_UNSUPPORTED_AST;
        return 0;
    }
    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        return 1;
    case AST_STMT_ASSIGNMENT:
        if (!emit_expression(context, statement->value.assignment.value, &value))
            return 0;
        success = emit(context, "%s = %s\n", statement->value.assignment.name, value);
        free(value);
        return success;
    case AST_STMT_BLOCK:
        return emit_statement_list(context, statement->value.block.statements);
    case AST_STMT_IF:
    case AST_STMT_IF_ELSE:
        return emit_if(context, statement);
    case AST_STMT_WHILE:
        return emit_while(context, statement);
    case AST_STMT_PRINT:
        if (!emit_expression(context, statement->value.print.value, &value))
            return 0;
        success = emit(context, "print %s\n", value);
        free(value);
        return success;
    }
    context->status = TAC_STATUS_UNSUPPORTED_AST;
    return 0;
}

TacStatus tac_generate(const AstProgram *program, FILE *output)
{
    TacContext context;
    if (program == NULL || output == NULL) return TAC_STATUS_INVALID_ARGUMENT;
    context.output = output;
    context.next_temporary = 1;
    context.next_label = 1;
    context.status = TAC_STATUS_OK;
    if (ferror(output)) return TAC_STATUS_OUTPUT_ERROR;
    if (!emit_statement_list(&context, program->statements) &&
        context.status == TAC_STATUS_OK)
        context.status = TAC_STATUS_UNSUPPORTED_AST;
    if (fflush(output) != 0 || ferror(output)) context.status = TAC_STATUS_OUTPUT_ERROR;
    return context.status;
}

const char *tac_status_message(TacStatus status)
{
    switch (status) {
    case TAC_STATUS_OK: return "success";
    case TAC_STATUS_INVALID_ARGUMENT: return "invalid TAC generator argument";
    case TAC_STATUS_OUT_OF_MEMORY: return "out of memory while generating TAC";
    case TAC_STATUS_OUTPUT_ERROR: return "failed to write TAC output";
    case TAC_STATUS_UNSUPPORTED_AST: return "unsupported AST node or operator";
    }
    return "unknown TAC generator error";
}
