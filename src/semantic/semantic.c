#include "semantic.h"

#include "../symbol_table/symbol_table.h"

#include <stdarg.h>
#include <stdio.h>

typedef enum ExpressionType {
    EXPRESSION_TYPE_ERROR = -1,
    EXPRESSION_TYPE_INT,
    EXPRESSION_TYPE_FLOAT,
    EXPRESSION_TYPE_BOOL
} ExpressionType;

typedef struct SemanticContext {
    SymbolTable *symbols;
    FILE *diagnostics;
    int error_count;
} SemanticContext;

static const char *type_name(ExpressionType type)
{
    switch (type) {
    case EXPRESSION_TYPE_INT: return "int";
    case EXPRESSION_TYPE_FLOAT: return "float";
    case EXPRESSION_TYPE_BOOL: return "bool";
    case EXPRESSION_TYPE_ERROR: return "unknown";
    }
    return "unknown";
}

static ExpressionType expression_type_from_ast(AstType type)
{
    switch (type) {
    case AST_TYPE_INT: return EXPRESSION_TYPE_INT;
    case AST_TYPE_FLOAT: return EXPRESSION_TYPE_FLOAT;
    case AST_TYPE_BOOL: return EXPRESSION_TYPE_BOOL;
    }
    return EXPRESSION_TYPE_ERROR;
}

static ExpressionType expression_type_from_symbol(AstType type)
{
    return expression_type_from_ast(type);
}

static void report_error(SemanticContext *context, SourceLocation location,
                         const char *format, ...)
{
    va_list arguments;
    fprintf(context->diagnostics, "Semantic error at line %d: ", location.line);
    va_start(arguments, format);
    vfprintf(context->diagnostics, format, arguments);
    va_end(arguments);
    fputc('\n', context->diagnostics);
    ++context->error_count;
}

static int is_numeric(ExpressionType type)
{
    return type == EXPRESSION_TYPE_INT || type == EXPRESSION_TYPE_FLOAT;
}

static ExpressionType analyze_expression(SemanticContext *context,
                                         const AstExpr *expression);
static void analyze_statement(SemanticContext *context, const AstStmt *statement);

static ExpressionType analyze_binary(SemanticContext *context,
                                     const AstExpr *expression)
{
    AstBinaryOp op = expression->value.binary.op;
    ExpressionType left = analyze_expression(context, expression->value.binary.left);
    ExpressionType right = analyze_expression(context, expression->value.binary.right);

    if (left == EXPRESSION_TYPE_ERROR || right == EXPRESSION_TYPE_ERROR)
        return EXPRESSION_TYPE_ERROR;

    switch (op) {
    case AST_OP_ADD:
    case AST_OP_SUBTRACT:
    case AST_OP_MULTIPLY:
    case AST_OP_DIVIDE:
        if (!is_numeric(left) || !is_numeric(right)) {
            report_error(context, expression->location,
                         "Operator '%s' requires numeric operands, but found %s and %s.",
                         ast_binary_op_name(op), type_name(left), type_name(right));
            return EXPRESSION_TYPE_ERROR;
        }
        return left == EXPRESSION_TYPE_FLOAT || right == EXPRESSION_TYPE_FLOAT
            ? EXPRESSION_TYPE_FLOAT : EXPRESSION_TYPE_INT;

    case AST_OP_MODULO:
        if (left != EXPRESSION_TYPE_INT || right != EXPRESSION_TYPE_INT) {
            report_error(context, expression->location,
                         "Operator '%%' requires integer operands, but found %s and %s.",
                         type_name(left), type_name(right));
            return EXPRESSION_TYPE_ERROR;
        }
        return EXPRESSION_TYPE_INT;

    case AST_OP_LESS:
    case AST_OP_GREATER:
    case AST_OP_LESS_EQUAL:
    case AST_OP_GREATER_EQUAL:
        if (!is_numeric(left) || !is_numeric(right)) {
            report_error(context, expression->location,
                         "Operator '%s' requires numeric operands, but found %s and %s.",
                         ast_binary_op_name(op), type_name(left), type_name(right));
            return EXPRESSION_TYPE_ERROR;
        }
        return EXPRESSION_TYPE_BOOL;

    case AST_OP_EQUAL:
    case AST_OP_NOT_EQUAL:
        if (left == right || (is_numeric(left) && is_numeric(right)))
            return EXPRESSION_TYPE_BOOL;
        report_error(context, expression->location,
                     "Operator '%s' cannot compare %s with %s.",
                     ast_binary_op_name(op), type_name(left), type_name(right));
        return EXPRESSION_TYPE_ERROR;

    case AST_OP_AND:
    case AST_OP_OR:
        if (left != EXPRESSION_TYPE_BOOL || right != EXPRESSION_TYPE_BOOL) {
            report_error(context, expression->location,
                         "Operator '%s' requires bool operands, but found %s and %s.",
                         ast_binary_op_name(op), type_name(left), type_name(right));
            return EXPRESSION_TYPE_ERROR;
        }
        return EXPRESSION_TYPE_BOOL;
    }

    report_error(context, expression->location, "Unsupported binary operator in AST.");
    return EXPRESSION_TYPE_ERROR;
}

static ExpressionType analyze_expression(SemanticContext *context,
                                         const AstExpr *expression)
{
    const SymbolEntry *entry;
    ExpressionType operand;
    if (expression == NULL) return EXPRESSION_TYPE_ERROR;

    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        entry = symbol_table_lookup(context->symbols, expression->value.identifier);
        if (entry == NULL) {
            report_error(context, expression->location,
                         "Undeclared variable '%s'.", expression->value.identifier);
            return EXPRESSION_TYPE_ERROR;
        }
        return expression_type_from_symbol(entry->type);

    case AST_EXPR_INTEGER:
        return EXPRESSION_TYPE_INT;
    case AST_EXPR_FLOAT:
        return EXPRESSION_TYPE_FLOAT;
    case AST_EXPR_BOOLEAN:
        return EXPRESSION_TYPE_BOOL;
    case AST_EXPR_BINARY:
        return analyze_binary(context, expression);
    case AST_EXPR_UNARY:
        operand = analyze_expression(context, expression->value.unary.operand);
        if (operand == EXPRESSION_TYPE_ERROR) return EXPRESSION_TYPE_ERROR;
        if (expression->value.unary.op == AST_OP_NOT) {
            if (operand != EXPRESSION_TYPE_BOOL) {
                report_error(context, expression->location,
                             "Operator '!' requires a bool operand, but found %s.",
                             type_name(operand));
                return EXPRESSION_TYPE_ERROR;
            }
            return EXPRESSION_TYPE_BOOL;
        }
        report_error(context, expression->location, "Unsupported unary operator in AST.");
        return EXPRESSION_TYPE_ERROR;
    }

    report_error(context, expression->location, "Unsupported expression node in AST.");
    return EXPRESSION_TYPE_ERROR;
}

static void analyze_statement_list(SemanticContext *context,
                                   const AstStmtList *list)
{
    for (; list != NULL; list = list->next)
        analyze_statement(context, list->statement);
}

static void analyze_block(SemanticContext *context, const AstStmt *block)
{
    if (symbol_table_enter_scope(context->symbols, block->location) < 0) {
        report_error(context, block->location, "Unable to enter block scope (out of memory).");
        return;
    }
    analyze_statement_list(context, block->value.block.statements);
    symbol_table_exit_scope(context->symbols);
}

static void analyze_statement(SemanticContext *context, const AstStmt *statement)
{
    SymbolTableStatus status;
    const SymbolEntry *target;
    ExpressionType value_type;
    if (statement == NULL) return;

    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        status = symbol_table_declare(context->symbols,
                                      statement->value.declaration.name,
                                      statement->value.declaration.type,
                                      statement->location.line, NULL);
        if (status == SYMBOL_TABLE_DUPLICATE) {
            report_error(context, statement->location,
                         "Duplicate declaration of '%s' in the same scope.",
                         statement->value.declaration.name);
        } else if (status != SYMBOL_TABLE_OK) {
            report_error(context, statement->location,
                         "Unable to add declaration of '%s' to the symbol table.",
                         statement->value.declaration.name);
        }
        break;

    case AST_STMT_ASSIGNMENT:
        target = symbol_table_lookup(context->symbols,
                                     statement->value.assignment.name);
        value_type = analyze_expression(context, statement->value.assignment.value);
        if (target == NULL) {
            report_error(context, statement->location,
                         "Undeclared variable '%s' used as assignment target.",
                         statement->value.assignment.name);
        } else if (value_type != EXPRESSION_TYPE_ERROR) {
            ExpressionType target_type = expression_type_from_symbol(target->type);
            int compatible = target_type == value_type ||
                (target_type == EXPRESSION_TYPE_FLOAT && value_type == EXPRESSION_TYPE_INT);
            if (!compatible) {
                report_error(context, statement->location,
                             "Cannot assign %s to %s variable '%s'.",
                             type_name(value_type), type_name(target_type),
                             statement->value.assignment.name);
            }
        }
        break;

    case AST_STMT_BLOCK:
        analyze_block(context, statement);
        break;

    case AST_STMT_IF:
    case AST_STMT_IF_ELSE:
        value_type = analyze_expression(context,
                                        statement->value.conditional.condition);
        if (value_type != EXPRESSION_TYPE_ERROR && value_type != EXPRESSION_TYPE_BOOL) {
            report_error(context, statement->value.conditional.condition->location,
                         "If condition must be bool, but found %s.",
                         type_name(value_type));
        }
        analyze_statement(context, statement->value.conditional.then_branch);
        analyze_statement(context, statement->value.conditional.else_branch);
        break;

    case AST_STMT_WHILE:
        value_type = analyze_expression(context, statement->value.loop.condition);
        if (value_type != EXPRESSION_TYPE_ERROR && value_type != EXPRESSION_TYPE_BOOL) {
            report_error(context, statement->value.loop.condition->location,
                         "While condition must be bool, but found %s.",
                         type_name(value_type));
        }
        analyze_statement(context, statement->value.loop.body);
        break;

    case AST_STMT_PRINT:
        (void)analyze_expression(context, statement->value.print.value);
        break;

    default:
        report_error(context, statement->location, "Unsupported statement node in AST.");
        break;
    }
}

SemanticResult semantic_analyze(const AstProgram *program, FILE *diagnostics)
{
    SemanticContext context;
    SemanticResult result;
    context.symbols = symbol_table_create();
    context.diagnostics = diagnostics != NULL ? diagnostics : stderr;
    context.error_count = 0;

    if (program == NULL || context.symbols == NULL) {
        fprintf(context.diagnostics,
                "Semantic error at line 1: Unable to initialize semantic analysis.\n");
        result.success = 0;
        result.error_count = 1;
        symbol_table_destroy(context.symbols);
        return result;
    }

    /* Visit one statement at a time so a declaration is visible only after it occurs. */
    analyze_statement_list(&context, program->statements);
    result.error_count = context.error_count;
    result.success = context.error_count == 0;
    symbol_table_destroy(context.symbols);
    return result;
}
