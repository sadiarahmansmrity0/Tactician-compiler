#include "tac.h"

#include "../parser/parser.h"
#include "../semantic/semantic.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    FILE *input;
    AstProgram *program = NULL;
    SemanticResult semantic_result;
    TacStatus tac_status;

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

    semantic_result = semantic_analyze(program, stderr);
    if (!semantic_result.success) {
        ast_program_free(program);
        return 1;
    }

    tac_status = tac_generate(program, stdout);
    ast_program_free(program);
    if (tac_status != TAC_STATUS_OK) {
        fprintf(stderr, "TAC error: %s.\n", tac_status_message(tac_status));
        return 1;
    }
    return 0;
}
