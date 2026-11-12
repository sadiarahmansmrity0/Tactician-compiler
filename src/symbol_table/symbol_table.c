#include "symbol_table.h"

#include <stdlib.h>
#include <string.h>

typedef struct SymbolScope {
    int id;
    int depth;
    SourceLocation opening_location;
    const AstStmt *owner_block;
    struct SymbolScope *parent;
    struct SymbolScope *first_child;
    struct SymbolScope *last_child;
    struct SymbolScope *next_sibling;
    SymbolEntry *first_symbol;
    SymbolEntry *last_symbol;
} SymbolScope;

struct SymbolTable {
    SymbolScope *root;
    SymbolScope *current;
    int next_scope_id;
};

static char *copy_string(const char *text)
{
    size_t length;
    char *copy;
    if (text == NULL) return NULL;
    length = strlen(text);
    copy = (char *)malloc(length + 1);
    if (copy != NULL) memcpy(copy, text, length + 1);
    return copy;
}

static SymbolScope *new_scope(int id, int depth, SourceLocation location,
                              SymbolScope *parent, const AstStmt *owner_block)
{
    SymbolScope *scope = (SymbolScope *)calloc(1, sizeof(SymbolScope));
    if (scope == NULL) return NULL;
    scope->id = id;
    scope->depth = depth;
    scope->opening_location = location;
    scope->parent = parent;
    scope->owner_block = owner_block;
    return scope;
}

SymbolTable *symbol_table_create(void)
{
    SymbolTable *table = (SymbolTable *)calloc(1, sizeof(SymbolTable));
    SourceLocation root_location = {1, 1};
    if (table == NULL) return NULL;
    table->root = new_scope(0, 0, root_location, NULL, NULL);
    if (table->root == NULL) {
        free(table);
        return NULL;
    }
    table->current = table->root;
    table->next_scope_id = 1;
    return table;
}

static void free_scope(SymbolScope *scope)
{
    SymbolScope *child;
    SymbolEntry *entry;
    if (scope == NULL) return;
    child = scope->first_child;
    while (child != NULL) {
        SymbolScope *next = child->next_sibling;
        free_scope(child);
        child = next;
    }
    entry = scope->first_symbol;
    while (entry != NULL) {
        SymbolEntry *next = entry->next;
        free(entry->name);
        free(entry);
        entry = next;
    }
    free(scope);
}

void symbol_table_destroy(SymbolTable *table)
{
    if (table == NULL) return;
    free_scope(table->root);
    free(table);
}

static int enter_scope(SymbolTable *table, SourceLocation opening_location,
                       const AstStmt *owner_block)
{
    SymbolScope *scope;
    if (table == NULL || table->current == NULL) return -1;
    scope = new_scope(table->next_scope_id, table->current->depth + 1,
                      opening_location, table->current, owner_block);
    if (scope == NULL) return -1;
    ++table->next_scope_id;
    if (table->current->last_child == NULL) {
        table->current->first_child = scope;
    } else {
        table->current->last_child->next_sibling = scope;
    }
    table->current->last_child = scope;
    table->current = scope;
    return scope->id;
}

int symbol_table_enter_scope(SymbolTable *table, SourceLocation opening_location)
{
    return enter_scope(table, opening_location, NULL);
}

int symbol_table_enter_existing_block(SymbolTable *table, const AstStmt *block)
{
    SymbolScope *child;
    if (table == NULL || table->current == NULL || block == NULL ||
        block->kind != AST_STMT_BLOCK) return -1;
    for (child = table->current->first_child; child != NULL;
         child = child->next_sibling) {
        if (child->owner_block == block) {
            table->current = child;
            return child->id;
        }
    }
    return -1;
}

int symbol_table_exit_scope(SymbolTable *table)
{
    if (table == NULL || table->current == NULL || table->current == table->root)
        return 0;
    table->current = table->current->parent;
    return 1;
}

int symbol_table_current_scope_id(const SymbolTable *table)
{
    if (table == NULL || table->current == NULL) return -1;
    return table->current->id;
}

const SymbolEntry *symbol_table_lookup_local(const SymbolTable *table,
                                             const char *name)
{
    SymbolEntry *entry;
    if (table == NULL || table->current == NULL || name == NULL) return NULL;
    for (entry = table->current->first_symbol; entry != NULL; entry = entry->next) {
        if (strcmp(entry->name, name) == 0) return entry;
    }
    return NULL;
}

const SymbolEntry *symbol_table_lookup_parent(const SymbolTable *table,
                                              const char *name)
{
    SymbolScope *scope;
    if (table == NULL || table->current == NULL || name == NULL) return NULL;
    for (scope = table->current->parent; scope != NULL; scope = scope->parent) {
        SymbolEntry *entry;
        for (entry = scope->first_symbol; entry != NULL; entry = entry->next) {
            if (strcmp(entry->name, name) == 0) return entry;
        }
    }
    return NULL;
}

const SymbolEntry *symbol_table_lookup(const SymbolTable *table,
                                       const char *name)
{
    const SymbolEntry *entry = symbol_table_lookup_local(table, name);
    if (entry != NULL) return entry;
    return symbol_table_lookup_parent(table, name);
}

SymbolTableStatus symbol_table_declare(SymbolTable *table, const char *name,
                                       AstType type, int declaration_line,
                                       const SymbolEntry **result)
{
    SymbolEntry *entry;
    if (result != NULL) *result = NULL;
    if (table == NULL || table->current == NULL || name == NULL || name[0] == '\0')
        return SYMBOL_TABLE_INVALID_ARGUMENT;
    entry = (SymbolEntry *)symbol_table_lookup_local(table, name);
    if (entry != NULL) {
        if (result != NULL) *result = entry;
        return SYMBOL_TABLE_DUPLICATE;
    }

    entry = (SymbolEntry *)calloc(1, sizeof(SymbolEntry));
    if (entry == NULL) return SYMBOL_TABLE_OUT_OF_MEMORY;
    entry->name = copy_string(name);
    if (entry->name == NULL) {
        free(entry);
        return SYMBOL_TABLE_OUT_OF_MEMORY;
    }
    entry->type = type;
    entry->scope_id = table->current->id;
    entry->declaration_line = declaration_line;
    if (table->current->last_symbol == NULL) {
        table->current->first_symbol = entry;
    } else {
        table->current->last_symbol->next = entry;
    }
    table->current->last_symbol = entry;
    if (result != NULL) *result = entry;
    return SYMBOL_TABLE_OK;
}

static int build_statement(SymbolTable *table, const AstStmt *statement,
                           FILE *diagnostics, int *duplicates);

static int build_statement_list(SymbolTable *table, const AstStmtList *list,
                                FILE *diagnostics, int *duplicates)
{
    for (; list != NULL; list = list->next) {
        if (!build_statement(table, list->statement, diagnostics, duplicates))
            return 0;
    }
    return 1;
}

static int build_block(SymbolTable *table, const AstStmt *block,
                       FILE *diagnostics, int *duplicates)
{
    int scope_id = enter_scope(table, block->location, block);
    int result;
    if (scope_id < 0) return 0;
    result = build_statement_list(table, block->value.block.statements,
                                  diagnostics, duplicates);
    symbol_table_exit_scope(table);
    return result;
}

static int build_statement(SymbolTable *table, const AstStmt *statement,
                           FILE *diagnostics, int *duplicates)
{
    const SymbolEntry *existing;
    SymbolTableStatus status;
    if (statement == NULL) return 1;
    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        status = symbol_table_declare(table,
                                      statement->value.declaration.name,
                                      statement->value.declaration.type,
                                      statement->location.line, &existing);
        if (status == SYMBOL_TABLE_DUPLICATE) {
            if (diagnostics == NULL) diagnostics = stderr;
            fprintf(diagnostics,
                    "%d: duplicate declaration of '%s' in scope %d\n",
                    statement->location.line,
                    statement->value.declaration.name, existing->scope_id);
            ++*duplicates;
            return 1;
        }
        return status == SYMBOL_TABLE_OK;
    case AST_STMT_BLOCK:
        return build_block(table, statement, diagnostics, duplicates);
    case AST_STMT_IF:
    case AST_STMT_IF_ELSE:
        if (!build_statement(table, statement->value.conditional.then_branch,
                             diagnostics, duplicates)) return 0;
        return build_statement(table, statement->value.conditional.else_branch,
                              diagnostics, duplicates);
    case AST_STMT_WHILE:
        return build_statement(table, statement->value.loop.body,
                               diagnostics, duplicates);
    case AST_STMT_ASSIGNMENT:
    case AST_STMT_PRINT:
        return 1;
    }
    return 1;
}

int symbol_table_build_from_ast(SymbolTable *table, const AstProgram *program,
                                FILE *diagnostics)
{
    int duplicates = 0;
    if (table == NULL || program == NULL || table->current != table->root)
        return -1;
    if (!build_statement_list(table, program->statements, diagnostics, &duplicates))
        return -1;
    return duplicates;
}

static void print_scope(const SymbolScope *scope, FILE *output)
{
    const SymbolEntry *entry;
    const SymbolScope *child;
    int i;
    for (i = 0; i < scope->depth; ++i) fputs("  ", output);
    if (scope->parent == NULL) {
        fprintf(output, "Scope %d (root)\n", scope->id);
    } else {
        fprintf(output, "Scope %d (parent %d, opened at %d:%d)\n",
                scope->id, scope->parent->id,
                scope->opening_location.line, scope->opening_location.column);
    }
    for (entry = scope->first_symbol; entry != NULL; entry = entry->next) {
        for (i = 0; i <= scope->depth; ++i) fputs("  ", output);
        fprintf(output, "%s : %s (declared line %d)\n", entry->name,
                ast_type_name(entry->type), entry->declaration_line);
    }
    for (child = scope->first_child; child != NULL; child = child->next_sibling)
        print_scope(child, output);
}

void symbol_table_print(const SymbolTable *table, FILE *output)
{
    if (table == NULL || output == NULL) return;
    print_scope(table->root, output);
}
