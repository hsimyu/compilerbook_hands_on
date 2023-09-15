#include <stdbool.h>
#include <stdio.h>

#include "util.h"
#include "tokenizer.h"
#include "parser.h"
#include "assembler.h"

// 入力プログラム
char *user_input;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Invalid argument count: arg must be 1.\n");
        return 1;
    }

    // トークン列を生成
    user_input = argv[1];
    tokenize(argv[1]);
    Node **nodes = parse();

    // アセンブリのヘッダー
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    int line = 0;
    while (nodes[line] != NULL)
    {
        // AST -> ASM
        gen(nodes[line]);
        line++;
    }

    return 0;
}