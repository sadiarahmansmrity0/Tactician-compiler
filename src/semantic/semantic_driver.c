#include "semantic.h"

#include "../parser/parser.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    FILE *input;
    AstProgram *program = NULL;
    SemanticResult result;

    if (argc != 2) {
        fprintf(stderr, "usage: %s source-file\n", argv[0]);
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror(argv[1]);
        return 2;
    }
    if (parser_parse(input, &program, stderr) != 0) {
        fclose(input);
        return 1;
    }
    fclose(input);

    result = semantic_analyze(program, stdout);
    ast_program_free(program);
    if (!result.success) {
        printf("Semantic analysis failed: %d error(s).\n", result.error_count);
        return 1;
    }
    puts("Semantic analysis passed: no errors.");
    return 0;
}
