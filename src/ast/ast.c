#include "ast.h"

#include <stdlib.h>

static AstExpr *new_expr(AstExprKind kind, SourceLocation location)
{
    AstExpr *expression = (AstExpr *)calloc(1, sizeof(AstExpr));
    if (expression != NULL) {
        expression->kind = kind;
        expression->location = location;
    }
    return expression;
}

static AstStmt *new_stmt(AstStmtKind kind, SourceLocation location)
{
    AstStmt *statement = (AstStmt *)calloc(1, sizeof(AstStmt));
    if (statement != NULL) {
        statement->kind = kind;
        statement->location = location;
    }
    return statement;
}

const char *ast_type_name(AstType type)
{
    switch (type) {
    case AST_TYPE_INT: return "int";
    case AST_TYPE_FLOAT: return "float";
    case AST_TYPE_BOOL: return "bool";
    }
    return "unknown";
}

const char *ast_binary_op_name(AstBinaryOp op)
{
    switch (op) {
    case AST_OP_ADD: return "+";
    case AST_OP_SUBTRACT: return "-";
    case AST_OP_MULTIPLY: return "*";
    case AST_OP_DIVIDE: return "/";
    case AST_OP_MODULO: return "%";
    case AST_OP_LESS: return "<";
    case AST_OP_GREATER: return ">";
    case AST_OP_LESS_EQUAL: return "<=";
    case AST_OP_GREATER_EQUAL: return ">=";
    case AST_OP_EQUAL: return "==";
    case AST_OP_NOT_EQUAL: return "!=";
    case AST_OP_AND: return "&&";
    case AST_OP_OR: return "||";
    }
    return "?";
}

AstProgram *ast_program_new(AstStmtList *statements, SourceLocation location)
{
    AstProgram *program = (AstProgram *)malloc(sizeof(AstProgram));
    if (program == NULL) return NULL;
    program->statements = statements;
    program->location = location;
    return program;
}

AstStmtList *ast_stmt_list_append(AstStmtList *list, AstStmt *statement)
{
    AstStmtList *item;
    AstStmtList *tail;
    if (statement == NULL) return list;
    item = (AstStmtList *)malloc(sizeof(AstStmtList));
    if (item == NULL) return NULL;
    item->statement = statement;
    item->next = NULL;
    item->location = statement->location;
    if (list == NULL) return item;
    tail = list;
    while (tail->next != NULL) tail = tail->next;
    tail->next = item;
    return list;
}

AstStmt *ast_stmt_declaration(AstType type, char *name, SourceLocation location)
{
    AstStmt *statement = new_stmt(AST_STMT_DECLARATION, location);
    if (statement == NULL) return NULL;
    statement->value.declaration.type = type;
    statement->value.declaration.name = name;
    return statement;
}

AstStmt *ast_stmt_assignment(char *name, AstExpr *value, SourceLocation location)
{
    AstStmt *statement = new_stmt(AST_STMT_ASSIGNMENT, location);
    if (statement == NULL) return NULL;
    statement->value.assignment.name = name;
    statement->value.assignment.value = value;
    return statement;
}

AstStmt *ast_stmt_block(AstStmtList *statements, SourceLocation location)
{
    AstStmt *statement = new_stmt(AST_STMT_BLOCK, location);
    if (statement == NULL) return NULL;
    statement->value.block.statements = statements;
    return statement;
}

AstStmt *ast_stmt_if(AstExpr *condition, AstStmt *then_branch, SourceLocation location)
{
    AstStmt *statement = new_stmt(AST_STMT_IF, location);
    if (statement == NULL) return NULL;
    statement->value.conditional.condition = condition;
    statement->value.conditional.then_branch = then_branch;
    statement->value.conditional.else_branch = NULL;
    return statement;
}

AstStmt *ast_stmt_if_else(AstExpr *condition, AstStmt *then_branch,
                          AstStmt *else_branch, SourceLocation location)
{
    AstStmt *statement = new_stmt(AST_STMT_IF_ELSE, location);
    if (statement == NULL) return NULL;
    statement->value.conditional.condition = condition;
    statement->value.conditional.then_branch = then_branch;
    statement->value.conditional.else_branch = else_branch;
    return statement;
}

AstStmt *ast_stmt_while(AstExpr *condition, AstStmt *body, SourceLocation location)
{
    AstStmt *statement = new_stmt(AST_STMT_WHILE, location);
    if (statement == NULL) return NULL;
    statement->value.loop.condition = condition;
    statement->value.loop.body = body;
    return statement;
}

AstStmt *ast_stmt_print(AstExpr *value, SourceLocation location)
{
    AstStmt *statement = new_stmt(AST_STMT_PRINT, location);
    if (statement == NULL) return NULL;
    statement->value.print.value = value;
    return statement;
}

AstExpr *ast_expr_identifier(char *name, SourceLocation location)
{
    AstExpr *expression = new_expr(AST_EXPR_IDENTIFIER, location);
    if (expression == NULL) return NULL;
    expression->value.identifier = name;
    return expression;
}

AstExpr *ast_expr_integer(long long value, SourceLocation location)
{
    AstExpr *expression = new_expr(AST_EXPR_INTEGER, location);
    if (expression != NULL) expression->value.integer = value;
    return expression;
}

AstExpr *ast_expr_float(double value, SourceLocation location)
{
    AstExpr *expression = new_expr(AST_EXPR_FLOAT, location);
    if (expression != NULL) expression->value.floating = value;
    return expression;
}

AstExpr *ast_expr_boolean(int value, SourceLocation location)
{
    AstExpr *expression = new_expr(AST_EXPR_BOOLEAN, location);
    if (expression != NULL) expression->value.boolean = value != 0;
    return expression;
}

AstExpr *ast_expr_binary(AstBinaryOp op, AstExpr *left, AstExpr *right,
                         SourceLocation location)
{
    AstExpr *expression = new_expr(AST_EXPR_BINARY, location);
    if (expression == NULL) return NULL;
    expression->value.binary.op = op;
    expression->value.binary.left = left;
    expression->value.binary.right = right;
    return expression;
}

AstExpr *ast_expr_unary(AstUnaryOp op, AstExpr *operand, SourceLocation location)
{
    AstExpr *expression = new_expr(AST_EXPR_UNARY, location);
    if (expression == NULL) return NULL;
    expression->value.unary.op = op;
    expression->value.unary.operand = operand;
    return expression;
}

void ast_expr_free(AstExpr *expression)
{
    if (expression == NULL) return;
    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        free(expression->value.identifier);
        break;
    case AST_EXPR_BINARY:
        ast_expr_free(expression->value.binary.left);
        ast_expr_free(expression->value.binary.right);
        break;
    case AST_EXPR_UNARY:
        ast_expr_free(expression->value.unary.operand);
        break;
    case AST_EXPR_INTEGER:
    case AST_EXPR_FLOAT:
    case AST_EXPR_BOOLEAN:
        break;
    }
    free(expression);
}

void ast_stmt_free(AstStmt *statement)
{
    if (statement == NULL) return;
    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        free(statement->value.declaration.name);
        break;
    case AST_STMT_ASSIGNMENT:
        free(statement->value.assignment.name);
        ast_expr_free(statement->value.assignment.value);
        break;
    case AST_STMT_BLOCK:
        ast_stmt_list_free(statement->value.block.statements);
        break;
    case AST_STMT_IF:
    case AST_STMT_IF_ELSE:
        ast_expr_free(statement->value.conditional.condition);
        ast_stmt_free(statement->value.conditional.then_branch);
        ast_stmt_free(statement->value.conditional.else_branch);
        break;
    case AST_STMT_WHILE:
        ast_expr_free(statement->value.loop.condition);
        ast_stmt_free(statement->value.loop.body);
        break;
    case AST_STMT_PRINT:
        ast_expr_free(statement->value.print.value);
        break;
    }
    free(statement);
}

void ast_stmt_list_free(AstStmtList *list)
{
    while (list != NULL) {
        AstStmtList *next = list->next;
        ast_stmt_free(list->statement);
        free(list);
        list = next;
    }
}

void ast_program_free(AstProgram *program)
{
    if (program == NULL) return;
    ast_stmt_list_free(program->statements);
    free(program);
}

static void indent(FILE *output, int depth)
{
    while (depth-- > 0) fputs("  ", output);
}

static void print_expr(const AstExpr *expression, FILE *output, int depth)
{
    if (expression == NULL) return;
    indent(output, depth);
    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        fprintf(output, "Identifier (%s)\n", expression->value.identifier);
        break;
    case AST_EXPR_INTEGER:
        fprintf(output, "Integer (%lld)\n", expression->value.integer);
        break;
    case AST_EXPR_FLOAT:
        fprintf(output, "Float (%.15g)\n", expression->value.floating);
        break;
    case AST_EXPR_BOOLEAN:
        fprintf(output, "Boolean (%s)\n", expression->value.boolean ? "true" : "false");
        break;
    case AST_EXPR_BINARY:
        fprintf(output, "Binary (%s)\n", ast_binary_op_name(expression->value.binary.op));
        print_expr(expression->value.binary.left, output, depth + 1);
        print_expr(expression->value.binary.right, output, depth + 1);
        break;
    case AST_EXPR_UNARY:
        fputs("Unary (!)\n", output);
        print_expr(expression->value.unary.operand, output, depth + 1);
        break;
    }
}

static void print_stmt(const AstStmt *statement, FILE *output, int depth)
{
    const AstStmtList *item;
    if (statement == NULL) return;
    indent(output, depth);
    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        fprintf(output, "Declaration (%s %s)\n", ast_type_name(statement->value.declaration.type),
                statement->value.declaration.name);
        break;
    case AST_STMT_ASSIGNMENT:
        fprintf(output, "Assignment (%s)\n", statement->value.assignment.name);
        print_expr(statement->value.assignment.value, output, depth + 1);
        break;
    case AST_STMT_BLOCK:
        fputs("Block\n", output);
        for (item = statement->value.block.statements; item != NULL; item = item->next)
            print_stmt(item->statement, output, depth + 1);
        break;
    case AST_STMT_IF:
    case AST_STMT_IF_ELSE:
        fputs(statement->kind == AST_STMT_IF ? "If\n" : "IfElse\n", output);
        indent(output, depth + 1); fputs("Condition\n", output);
        print_expr(statement->value.conditional.condition, output, depth + 2);
        indent(output, depth + 1); fputs("Then\n", output);
        print_stmt(statement->value.conditional.then_branch, output, depth + 2);
        if (statement->value.conditional.else_branch != NULL) {
            indent(output, depth + 1); fputs("Else\n", output);
            print_stmt(statement->value.conditional.else_branch, output, depth + 2);
        }
        break;
    case AST_STMT_WHILE:
        fputs("While\n", output);
        indent(output, depth + 1); fputs("Condition\n", output);
        print_expr(statement->value.loop.condition, output, depth + 2);
        indent(output, depth + 1); fputs("Body\n", output);
        print_stmt(statement->value.loop.body, output, depth + 2);
        break;
    case AST_STMT_PRINT:
        fputs("Print\n", output);
        print_expr(statement->value.print.value, output, depth + 1);
        break;
    }
}

void ast_print(const AstProgram *program, FILE *output)
{
    const AstStmtList *item;
    if (program == NULL || output == NULL) return;
    fputs("Program\n", output);
    for (item = program->statements; item != NULL; item = item->next)
        print_stmt(item->statement, output, 1);
}
