%{
#include "parser.h"
#include "lexer_adapter.h"

#include <stdlib.h>

static AstProgram *parser_root = NULL;
static int syntax_errors = 0;
static FILE *parser_diagnostics = NULL;

static void report_memory_error(void);
int yylex(void);
void yyerror(const char *message);
#define SOURCE_LOCATION(location) \
    ((SourceLocation){ (location).first_line, (location).first_column })
%}

%code requires {
#include "ast.h"
}

%locations
%error-verbose

%union {
    char *text;
    long long integer_value;
    double float_value;
    int boolean_value;
    AstType type;
    AstBinaryOp binary_op;
    AstUnaryOp unary_op;
    AstExpr *expression;
    AstStmt *statement;
    AstStmtList *statement_list;
}

%token T_INT "int"
%token T_FLOAT "float"
%token T_BOOL "bool"
%token T_IF "if"
%token T_ELSE "else"
%token T_WHILE "while"
%token T_PRINT "print"
%token T_TRUE "true"
%token T_FALSE "false"
%token <text> T_IDENTIFIER "identifier"
%token <integer_value> T_INT_LITERAL "integer literal"
%token <float_value> T_FLOAT_LITERAL "floating-point literal"
%token T_ASSIGN "="
%token T_PLUS "+"
%token T_MINUS "-"
%token T_STAR "*"
%token T_SLASH "/"
%token T_PERCENT "%"
%token T_LT "<"
%token T_GT ">"
%token T_LE "<="
%token T_GE ">="
%token T_EQ "=="
%token T_NE "!="
%token T_AND "&&"
%token T_OR "||"
%token T_NOT "!"
%token T_LBRACE "{"
%token T_RBRACE "}"
%token T_LPAREN "("
%token T_RPAREN ")"
%token T_SEMICOLON ";"

%type <type> type
%type <statement_list> statement_list
%type <statement> statement declaration assignment if_statement while_statement print_statement block
%type <expression> expression logical_or logical_and equality relational additive multiplicative unary primary
%type <binary_op> binary_operator

%destructor { free($$); } <text>
%destructor { ast_expr_free($$); } <expression>
%destructor { ast_stmt_free($$); } <statement>
%destructor { ast_stmt_list_free($$); } <statement_list>

%%

program:
    statement_list
    {
        parser_root = ast_program_new($1, SOURCE_LOCATION(@$));
        if (parser_root == NULL) {
            report_memory_error();
            YYABORT;
        }
    }
;

statement_list:
    /* Empty programs and blocks are allowed. */
    {
        $$ = NULL;
    }
  | statement_list statement
    {
        if ($2 == NULL) {
            $$ = $1;
        } else {
            $$ = ast_stmt_list_append($1, $2);
            if ($$ == NULL) {
                ast_stmt_free($2);
                ast_stmt_list_free($1);
                report_memory_error();
                YYABORT;
            }
        }
    }
  | statement_list error T_SEMICOLON
    {
        $$ = $1;
        yyerrok;
    }
;

statement:
    declaration   { $$ = $1; }
  | assignment    { $$ = $1; }
  | if_statement  { $$ = $1; }
  | while_statement { $$ = $1; }
  | print_statement { $$ = $1; }
  | block         { $$ = $1; }
;

declaration:
    type T_IDENTIFIER T_SEMICOLON
    {
        $$ = ast_stmt_declaration($1, $2, SOURCE_LOCATION(@$));
        if ($$ == NULL) { free($2); report_memory_error(); YYABORT; }
    }
;

type:
    T_INT   { $$ = AST_TYPE_INT; }
  | T_FLOAT { $$ = AST_TYPE_FLOAT; }
  | T_BOOL  { $$ = AST_TYPE_BOOL; }
;

assignment:
    T_IDENTIFIER T_ASSIGN expression T_SEMICOLON
    {
        $$ = ast_stmt_assignment($1, $3, SOURCE_LOCATION(@$));
        if ($$ == NULL) { free($1); ast_expr_free($3); report_memory_error(); YYABORT; }
    }
;

if_statement:
    T_IF T_LPAREN expression T_RPAREN block
    {
        $$ = ast_stmt_if($3, $5, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($3); ast_stmt_free($5); report_memory_error(); YYABORT; }
    }
  | T_IF T_LPAREN expression T_RPAREN block T_ELSE block
    {
        $$ = ast_stmt_if_else($3, $5, $7, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($3); ast_stmt_free($5); ast_stmt_free($7); report_memory_error(); YYABORT; }
    }
;

while_statement:
    T_WHILE T_LPAREN expression T_RPAREN block
    {
        $$ = ast_stmt_while($3, $5, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($3); ast_stmt_free($5); report_memory_error(); YYABORT; }
    }
;

print_statement:
    T_PRINT expression T_SEMICOLON
    {
        $$ = ast_stmt_print($2, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($2); report_memory_error(); YYABORT; }
    }
;

block:
    T_LBRACE statement_list T_RBRACE
    {
        $$ = ast_stmt_block($2, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_stmt_list_free($2); report_memory_error(); YYABORT; }
    }
  | T_LBRACE statement_list error T_RBRACE
    {
        $$ = ast_stmt_block($2, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_stmt_list_free($2); report_memory_error(); YYABORT; }
        yyerrok;
    }
;

expression:
    logical_or { $$ = $1; }
;

logical_or:
    logical_or T_OR logical_and
    {
        $$ = ast_expr_binary(AST_OP_OR, $1, $3, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; }
    }
  | logical_and { $$ = $1; }
;

logical_and:
    logical_and T_AND equality
    {
        $$ = ast_expr_binary(AST_OP_AND, $1, $3, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; }
    }
  | equality { $$ = $1; }
;

equality:
    relational { $$ = $1; }
  | relational T_EQ relational
    {
        $$ = ast_expr_binary(AST_OP_EQUAL, $1, $3, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; }
    }
  | relational T_NE relational
    {
        $$ = ast_expr_binary(AST_OP_NOT_EQUAL, $1, $3, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; }
    }
;

relational:
    additive { $$ = $1; }
  | additive T_LT additive
    { $$ = ast_expr_binary(AST_OP_LESS, $1, $3, SOURCE_LOCATION(@$)); if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; } }
  | additive T_GT additive
    { $$ = ast_expr_binary(AST_OP_GREATER, $1, $3, SOURCE_LOCATION(@$)); if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; } }
  | additive T_LE additive
    { $$ = ast_expr_binary(AST_OP_LESS_EQUAL, $1, $3, SOURCE_LOCATION(@$)); if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; } }
  | additive T_GE additive
    { $$ = ast_expr_binary(AST_OP_GREATER_EQUAL, $1, $3, SOURCE_LOCATION(@$)); if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; } }
;

additive:
    additive binary_operator multiplicative
    {
        $$ = ast_expr_binary($2, $1, $3, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; }
    }
  | multiplicative { $$ = $1; }
;

binary_operator:
    T_PLUS    { $$ = AST_OP_ADD; }
  | T_MINUS   { $$ = AST_OP_SUBTRACT; }
;

multiplicative:
    multiplicative T_STAR unary
    { $$ = ast_expr_binary(AST_OP_MULTIPLY, $1, $3, SOURCE_LOCATION(@$)); if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; } }
  | multiplicative T_SLASH unary
    { $$ = ast_expr_binary(AST_OP_DIVIDE, $1, $3, SOURCE_LOCATION(@$)); if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; } }
  | multiplicative T_PERCENT unary
    { $$ = ast_expr_binary(AST_OP_MODULO, $1, $3, SOURCE_LOCATION(@$)); if ($$ == NULL) { ast_expr_free($1); ast_expr_free($3); report_memory_error(); YYABORT; } }
  | unary { $$ = $1; }
;

unary:
    T_NOT unary
    {
        $$ = ast_expr_unary(AST_OP_NOT, $2, SOURCE_LOCATION(@$));
        if ($$ == NULL) { ast_expr_free($2); report_memory_error(); YYABORT; }
    }
  | primary { $$ = $1; }
;

primary:
    T_IDENTIFIER
    {
        $$ = ast_expr_identifier($1, SOURCE_LOCATION(@$));
        if ($$ == NULL) { free($1); report_memory_error(); YYABORT; }
    }
  | T_INT_LITERAL
    {
        $$ = ast_expr_integer($1, SOURCE_LOCATION(@$));
        if ($$ == NULL) { report_memory_error(); YYABORT; }
    }
  | T_FLOAT_LITERAL
    {
        $$ = ast_expr_float($1, SOURCE_LOCATION(@$));
        if ($$ == NULL) { report_memory_error(); YYABORT; }
    }
  | T_TRUE
    {
        $$ = ast_expr_boolean(1, SOURCE_LOCATION(@$));
        if ($$ == NULL) { report_memory_error(); YYABORT; }
    }
  | T_FALSE
    {
        $$ = ast_expr_boolean(0, SOURCE_LOCATION(@$));
        if ($$ == NULL) { report_memory_error(); YYABORT; }
    }
  | T_LPAREN expression T_RPAREN { $$ = $2; }
;

%%

static void report_memory_error(void)
{
    if (parser_diagnostics != NULL)
        fputs("out of memory while constructing the syntax tree\n", parser_diagnostics);
    ++syntax_errors;
}

void yyerror(const char *message)
{
    const char *lexeme = parser_lexer_adapter_current_lexeme();
    ++syntax_errors;
    if (parser_diagnostics == NULL) parser_diagnostics = stderr;
    fprintf(parser_diagnostics, "%d:%d", yylloc.first_line,
            yylloc.first_column);
    if (lexeme != NULL && lexeme[0] != '\0')
        fprintf(parser_diagnostics, " near '%s'", lexeme);
    else
        fputs(" at end of file", parser_diagnostics);
    fprintf(parser_diagnostics, ": %s\n", message);
}

int parser_parse(FILE *input, AstProgram **result, FILE *diagnostics)
{
    int status;
    if (result == NULL || input == NULL) return 1;
    *result = NULL;
    parser_root = NULL;
    syntax_errors = 0;
    parser_diagnostics = diagnostics != NULL ? diagnostics : stderr;
    parser_lexer_adapter_reset(input, parser_diagnostics);

    status = yyparse();
    if (status != 0 || syntax_errors != 0 ||
        parser_lexer_adapter_error_count() != 0 || parser_root == NULL) {
        ast_program_free(parser_root);
        parser_root = NULL;
        return 1;
    }
    *result = parser_root;
    parser_root = NULL;
    return 0;
}
