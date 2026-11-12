#ifndef TACTICIAN_SYMBOL_TABLE_H
#define TACTICIAN_SYMBOL_TABLE_H

#include <stdio.h>
#include "../ast/ast.h"

typedef struct SymbolTable SymbolTable;

typedef struct SymbolEntry {
    char *name;
    AstType type;
    int scope_id;
    int declaration_line;
    struct SymbolEntry *next;
} SymbolEntry;

typedef enum SymbolTableStatus {
    SYMBOL_TABLE_OK,
    SYMBOL_TABLE_DUPLICATE,
    SYMBOL_TABLE_OUT_OF_MEMORY,
    SYMBOL_TABLE_INVALID_ARGUMENT
} SymbolTableStatus;

SymbolTable *symbol_table_create(void);
void symbol_table_destroy(SymbolTable *table);

/* Enter returns the new scope ID, or -1 if the table is invalid/out of memory. */
int symbol_table_enter_scope(SymbolTable *table, SourceLocation opening_location);
/* Re-enter the retained scope created for this AST block during the build pass. */
int symbol_table_enter_existing_block(SymbolTable *table, const AstStmt *block);
/* Returns 1 when a child scope was exited, or 0 when already at the root. */
int symbol_table_exit_scope(SymbolTable *table);
int symbol_table_current_scope_id(const SymbolTable *table);

SymbolTableStatus symbol_table_declare(SymbolTable *table, const char *name,
                                       AstType type, int declaration_line,
                                       const SymbolEntry **result);
const SymbolEntry *symbol_table_lookup_local(const SymbolTable *table,
                                             const char *name);
const SymbolEntry *symbol_table_lookup_parent(const SymbolTable *table,
                                              const char *name);
const SymbolEntry *symbol_table_lookup(const SymbolTable *table,
                                       const char *name);

/* Builds entries/scopes by traversing an AST. Returns duplicate count, or -1 on OOM. */
int symbol_table_build_from_ast(SymbolTable *table, const AstProgram *program,
                                FILE *diagnostics);
void symbol_table_print(const SymbolTable *table, FILE *output);

#endif
