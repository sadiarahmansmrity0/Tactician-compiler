#include "parser.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    FILE *input;
    AstProgram *program = NULL;
    int status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s source-file\n", argv[0]);
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror(argv[1]);
        return 2;
    }

    status = parser_parse(input, &program, stdout);
    fclose(input);
    if (status != 0) return 1;

    ast_print(program, stdout);
    ast_program_free(program);
    return 0;
}
