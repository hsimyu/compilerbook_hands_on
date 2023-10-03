#include <stdbool.h>
#include <stdio.h>

#include "util.h"
#include "tokenizer.h"
#include "parser.h"
#include "assembler.h"

char *user_input;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Invalid argument count: arg must be 1.\n");
        return 1;
    }

    // ファイルを読み込む
    char *file_path = argv[1];
    user_input = read_file(file_path);

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