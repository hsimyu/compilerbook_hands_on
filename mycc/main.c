#include <stdbool.h>
#include <stdio.h>

#include "util.h"
#include "tokenizer.h"
#include "parser.h"
#include "assembler.h"

char *user_input;
char *target_file_name;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Invalid argument count: arg must be 1.\n");
        return 1;
    }

    // ファイルを読み込む
    target_file_name = argv[1];
    user_input = read_file(target_file_name);

    // トークン列を生成
    tokenize(user_input);
    Node **nodes = parse();

    // アセンブリのヘッダー
    printf(".intel_syntax noprefix\n");

    // データセクションを生成
    gen_string_literals();

    int line = 0;
    while (nodes[line] != NULL)
    {
        // AST -> ASM
        gen(nodes[line]);
        line++;
    }

    return 0;
}