#include "symbol_table.h"
#include "../parser/parser.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    FILE *input;
    AstProgram *program = NULL;
    SymbolTable *table;
    int duplicate_count;
    int parse_status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s source-file\n", argv[0]);
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror(argv[1]);
        return 2;
    }
    parse_status = parser_parse(input, &program, stderr);
    fclose(input);
    if (parse_status != 0) return 1;

    table = symbol_table_create();
    if (table == NULL) {
        fputs("could not create symbol table\n", stderr);
        ast_program_free(program);
        return 2;
    }
    duplicate_count = symbol_table_build_from_ast(table, program, stdout);
    if (duplicate_count < 0) {
        fputs("could not build symbol table\n", stderr);
        symbol_table_destroy(table);
        ast_program_free(program);
        return 2;
    }

    symbol_table_print(table, stdout);
    symbol_table_destroy(table);
    ast_program_free(program);
    return duplicate_count == 0 ? 0 : 1;
}
