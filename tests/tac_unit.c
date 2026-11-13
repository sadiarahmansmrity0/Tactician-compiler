#include "../src/parser/parser.h"
#include "../src/semantic/semantic.h"
#include "../src/tac/tac.h"

#include <stdio.h>
#include <stdlib.h>

static int files_equal(const char *left_name, const char *right_name)
{
    FILE *left = fopen(left_name, "rb");
    FILE *right = fopen(right_name, "rb");
    int a;
    int b;
    int equal = 1;
    if (left == NULL || right == NULL) {
        if (left != NULL) fclose(left);
        if (right != NULL) fclose(right);
        return 0;
    }
    do {
        a = fgetc(left);
        b = fgetc(right);
        if (a != b) {
            equal = 0;
            break;
        }
    } while (a != EOF);
    fclose(left);
    fclose(right);
    return equal;
}

static int generate_to(const AstProgram *program, const char *path)
{
    FILE *output = fopen(path, "wb");
    TacStatus status;
    if (output == NULL) return 0;
    status = tac_generate(program, output);
    fclose(output);
    return status == TAC_STATUS_OK;
}

int main(void)
{
    FILE *input = fopen("tests/tac/valid/expressions.tac", "rb");
    AstProgram *program = NULL;
    SemanticResult semantic;
    int success = 0;
    if (input == NULL || parser_parse(input, &program, stderr) != 0) goto cleanup;
    fclose(input);
    input = NULL;
    semantic = semantic_analyze(program, stderr);
    if (!semantic.success) goto cleanup;
    if (!generate_to(program, "build/tac-unit-first.actual") ||
        !generate_to(program, "build/tac-unit-second.actual")) goto cleanup;
    if (!files_equal("build/tac-unit-first.actual", "build/tac-unit-second.actual"))
        goto cleanup;
    success = 1;

cleanup:
    if (input != NULL) fclose(input);
    ast_program_free(program);
    if (success) puts("PASS TAC generator resets counters and is deterministic");
    else puts("FAIL TAC generator repeat-generation check");
    return success ? 0 : 1;
}
