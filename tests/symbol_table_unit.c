#include "../src/symbol_table/symbol_table.h"
#include "../src/parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

static void check(int condition, const char *description)
{
    if (condition) {
        printf("PASS %s\n", description);
    } else {
        printf("FAIL %s\n", description);
        ++failures;
    }
}

static AstStmt *find_stmt(AstStmtList *list, AstStmtKind kind)
{
    while (list != NULL) {
        if (list->statement != NULL && list->statement->kind == kind)
            return list->statement;
        list = list->next;
    }
    return NULL;
}

static int test_ast_scope_mapping(const char *path)
{
    FILE *input = fopen(path, "rb");
    FILE *diagnostics = fopen("build/symbol-table-ast-diagnostics.txt", "w+");
    AstProgram *program = NULL;
    SymbolTable *table;
    AstStmt *outer_block;
    AstStmt *if_statement;
    AstStmt *if_block;
    AstStmt *nested_block;
    const SymbolEntry *entry;
    int duplicates;
    int ok = 1;

    if (input == NULL || diagnostics == NULL) {
        if (input != NULL) fclose(input);
        if (diagnostics != NULL) fclose(diagnostics);
        check(0, "AST scope test setup");
        return 0;
    }
    if (parser_parse(input, &program, diagnostics) != 0) {
        fclose(input);
        fclose(diagnostics);
        check(0, "parse AST for scope integration");
        return 0;
    }
    fclose(input);
    table = symbol_table_create();
    if (table == NULL) {
        ast_program_free(program);
        fclose(diagnostics);
        check(0, "create symbol table for AST integration");
        return 0;
    }
    duplicates = symbol_table_build_from_ast(table, program, diagnostics);
    outer_block = find_stmt(program->statements, AST_STMT_BLOCK);
    if_statement = find_stmt(program->statements, AST_STMT_IF);
    if_block = if_statement != NULL ? if_statement->value.conditional.then_branch : NULL;
    nested_block = outer_block != NULL ?
        find_stmt(outer_block->value.block.statements, AST_STMT_BLOCK) : NULL;
    check(duplicates == 0, "AST traversal inserts declarations without duplicates");

    check(outer_block != NULL && symbol_table_enter_existing_block(table, outer_block) > 0,
          "re-enter AST block's retained scope");
    entry = symbol_table_lookup(table, "y");
    check(entry != NULL && entry->type == AST_TYPE_FLOAT,
          "AST block scope contains its float declaration");
    check(nested_block != NULL && symbol_table_enter_existing_block(table, nested_block) > 0,
          "re-enter nested AST block scope");
    entry = symbol_table_lookup(table, "z");
    check(entry != NULL && entry->type == AST_TYPE_BOOL,
          "nested AST scope resolves its bool declaration");
    symbol_table_exit_scope(table);
    check(symbol_table_lookup(table, "z") == NULL,
          "nested AST declaration is hidden after scope exit");
    symbol_table_exit_scope(table);
    check(symbol_table_lookup(table, "y") == NULL,
          "outer block declaration is hidden after scope exit");

    check(if_block != NULL && symbol_table_enter_existing_block(table, if_block) > 0,
          "if body has a retained child scope");
    entry = symbol_table_lookup(table, "x");
    check(entry != NULL && entry->type == AST_TYPE_FLOAT,
          "if body shadows the root int declaration");
    symbol_table_exit_scope(table);

    symbol_table_destroy(table);
    ast_program_free(program);
    fclose(diagnostics);
    return ok;
}

static int test_manual_scope_api(void)
{
    SymbolTable *table = symbol_table_create();
    SourceLocation location = {1, 1};
    const SymbolEntry *entry = NULL;
    SymbolTableStatus status;
    int child_id;
    int nested_id;
    int good = 1;
    FILE *dump;
    char output[2048];
    size_t read_count;

    if (table == NULL) {
        check(0, "create symbol table");
        return 0;
    }
    check(symbol_table_current_scope_id(table) == 0, "root scope starts at ID 0");
    check(!symbol_table_exit_scope(table), "cannot exit the root scope");

    status = symbol_table_declare(table, "rootInt", AST_TYPE_INT, 2, &entry);
    check(status == SYMBOL_TABLE_OK && entry != NULL && entry->type == AST_TYPE_INT &&
          entry->scope_id == 0 && entry->declaration_line == 2,
          "insert int with scope and declaration line");
    status = symbol_table_declare(table, "rootFloat", AST_TYPE_FLOAT, 3, &entry);
    check(status == SYMBOL_TABLE_OK && entry != NULL && entry->type == AST_TYPE_FLOAT,
          "insert float");
    status = symbol_table_declare(table, "rootBool", AST_TYPE_BOOL, 4, &entry);
    check(status == SYMBOL_TABLE_OK && entry != NULL && entry->type == AST_TYPE_BOOL,
          "insert bool");
    check(symbol_table_lookup(table, "rootInt") != NULL, "lookup existing root variable");
    check(symbol_table_lookup(table, "missing") == NULL, "lookup missing variable");
    check(symbol_table_declare(table, "rootInt", AST_TYPE_BOOL, 9, &entry) ==
          SYMBOL_TABLE_DUPLICATE && entry->type == AST_TYPE_INT,
          "reject duplicate declaration in same scope");

    child_id = symbol_table_enter_scope(table, location);
    check(child_id == 1 && symbol_table_lookup_local(table, "rootInt") == NULL,
          "enter child scope with no local copy of parent name");
    check(symbol_table_lookup_parent(table, "rootInt") != NULL &&
          symbol_table_lookup(table, "rootInt") != NULL,
          "child resolves a parent-scope declaration");
    check(symbol_table_declare(table, "rootInt", AST_TYPE_FLOAT, 6, &entry) ==
          SYMBOL_TABLE_OK && entry->scope_id == child_id,
          "allow child scope to shadow a parent name");
    check(symbol_table_lookup(table, "rootInt")->type == AST_TYPE_FLOAT &&
          symbol_table_lookup_parent(table, "rootInt")->type == AST_TYPE_INT,
          "local lookup wins while parent lookup remains available");
    check(symbol_table_declare(table, "childOnly", AST_TYPE_BOOL, 7, NULL) ==
          SYMBOL_TABLE_OK, "insert child-only name");

    nested_id = symbol_table_enter_scope(table, location);
    check(nested_id == 2 && symbol_table_lookup(table, "childOnly") != NULL,
          "nested scope resolves its parent's local declaration");
    check(symbol_table_declare(table, "deepOnly", AST_TYPE_INT, 8, NULL) ==
          SYMBOL_TABLE_OK, "insert nested-only name");
    check(symbol_table_exit_scope(table) && symbol_table_current_scope_id(table) == child_id,
          "exit nested scope restores its parent");
    check(symbol_table_lookup(table, "deepOnly") == NULL &&
          symbol_table_lookup(table, "childOnly") != NULL,
          "nested-only name disappears while parent names remain");
    check(symbol_table_exit_scope(table) && symbol_table_current_scope_id(table) == 0,
          "exit child scope restores root");
    check(symbol_table_lookup(table, "childOnly") == NULL &&
          symbol_table_lookup(table, "deepOnly") == NULL &&
          symbol_table_lookup(table, "rootInt")->type == AST_TYPE_INT,
          "child names are hidden and root binding is restored");

    location.line = 10;
    check(symbol_table_enter_scope(table, location) == 3,
          "new sibling scope receives a distinct ID");
    check(symbol_table_lookup(table, "deepOnly") == NULL,
          "sibling scope cannot see an exited sibling's declarations");
    symbol_table_exit_scope(table);

    dump = fopen("build/symbol-table-unit-dump.txt", "w+");
    if (dump != NULL) {
        symbol_table_print(table, dump);
        rewind(dump);
        read_count = fread(output, 1, sizeof(output) - 1, dump);
        output[read_count] = '\0';
        check(strstr(output, "Scope 0 (root)") != NULL &&
              strstr(output, "rootFloat : float (declared line 3)") != NULL &&
              strstr(output, "Scope 2 (parent 1") != NULL,
              "print readable scope hierarchy and symbol details");
        fclose(dump);
    } else {
        check(0, "open temporary stream for table output");
        good = 0;
    }

    symbol_table_destroy(table);
    return good;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s ast-test-source\n", argv[0]);
        return 2;
    }
    test_manual_scope_api();
    test_ast_scope_mapping(argv[1]);
    if (failures != 0) {
        printf("%d symbol-table checks failed\n", failures);
        return 1;
    }
    puts("All symbol-table unit checks passed");
    return 0;
}
