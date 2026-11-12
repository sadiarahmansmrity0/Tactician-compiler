#ifndef TACTICIAN_AST_H
#define TACTICIAN_AST_H

#include <stdio.h>
#include "../common/source_location.h"

typedef enum AstType {
    AST_TYPE_INT,
    AST_TYPE_FLOAT,
    AST_TYPE_BOOL
} AstType;

typedef enum AstStmtKind {
    AST_STMT_DECLARATION,
    AST_STMT_ASSIGNMENT,
    AST_STMT_BLOCK,
    AST_STMT_IF,
    AST_STMT_IF_ELSE,
    AST_STMT_WHILE,
    AST_STMT_PRINT
} AstStmtKind;

typedef enum AstExprKind {
    AST_EXPR_IDENTIFIER,
    AST_EXPR_INTEGER,
    AST_EXPR_FLOAT,
    AST_EXPR_BOOLEAN,
    AST_EXPR_BINARY,
    AST_EXPR_UNARY
} AstExprKind;

typedef enum AstBinaryOp {
    AST_OP_ADD,
    AST_OP_SUBTRACT,
    AST_OP_MULTIPLY,
    AST_OP_DIVIDE,
    AST_OP_MODULO,
    AST_OP_LESS,
    AST_OP_GREATER,
    AST_OP_LESS_EQUAL,
    AST_OP_GREATER_EQUAL,
    AST_OP_EQUAL,
    AST_OP_NOT_EQUAL,
    AST_OP_AND,
    AST_OP_OR
} AstBinaryOp;

typedef enum AstUnaryOp {
    AST_OP_NOT
} AstUnaryOp;

typedef struct AstExpr AstExpr;
typedef struct AstStmt AstStmt;
typedef struct AstStmtList AstStmtList;
typedef struct AstProgram AstProgram;

struct AstExpr {
    AstExprKind kind;
    SourceLocation location;
    union {
        char *identifier;
        long long integer;
        double floating;
        int boolean;
        struct { AstBinaryOp op; AstExpr *left; AstExpr *right; } binary;
        struct { AstUnaryOp op; AstExpr *operand; } unary;
    } value;
};

struct AstStmt {
    AstStmtKind kind;
    SourceLocation location;
    union {
        struct { AstType type; char *name; } declaration;
        struct { char *name; AstExpr *value; } assignment;
        struct { AstStmtList *statements; } block;
        struct { AstExpr *condition; AstStmt *then_branch; AstStmt *else_branch; } conditional;
        struct { AstExpr *condition; AstStmt *body; } loop;
        struct { AstExpr *value; } print;
    } value;
};

struct AstStmtList {
    AstStmt *statement;
    AstStmtList *next;
    SourceLocation location;
};

struct AstProgram {
    AstStmtList *statements;
    SourceLocation location;
};

AstProgram *ast_program_new(AstStmtList *statements, SourceLocation location);
AstStmtList *ast_stmt_list_append(AstStmtList *list, AstStmt *statement);
AstStmt *ast_stmt_declaration(AstType type, char *name, SourceLocation location);
AstStmt *ast_stmt_assignment(char *name, AstExpr *value, SourceLocation location);
AstStmt *ast_stmt_block(AstStmtList *statements, SourceLocation location);
AstStmt *ast_stmt_if(AstExpr *condition, AstStmt *then_branch,
                     SourceLocation location);
AstStmt *ast_stmt_if_else(AstExpr *condition, AstStmt *then_branch,
                          AstStmt *else_branch, SourceLocation location);
AstStmt *ast_stmt_while(AstExpr *condition, AstStmt *body, SourceLocation location);
AstStmt *ast_stmt_print(AstExpr *value, SourceLocation location);
AstExpr *ast_expr_identifier(char *name, SourceLocation location);
AstExpr *ast_expr_integer(long long value, SourceLocation location);
AstExpr *ast_expr_float(double value, SourceLocation location);
AstExpr *ast_expr_boolean(int value, SourceLocation location);
AstExpr *ast_expr_binary(AstBinaryOp op, AstExpr *left, AstExpr *right,
                         SourceLocation location);
AstExpr *ast_expr_unary(AstUnaryOp op, AstExpr *operand, SourceLocation location);

void ast_program_free(AstProgram *program);
void ast_stmt_free(AstStmt *statement);
void ast_stmt_list_free(AstStmtList *list);
void ast_expr_free(AstExpr *expression);
void ast_print(const AstProgram *program, FILE *output);
const char *ast_type_name(AstType type);
const char *ast_binary_op_name(AstBinaryOp op);

#endif
