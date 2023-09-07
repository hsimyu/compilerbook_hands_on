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
    printf("main:\n");

    // プロローグ
    // スタックとして変数 26 個分の領域を確保
    printf("  push rbp\n");     // rbp の値をリターンアドレスとして保存
    printf("  mov rbp, rsp\n"); // 関数突入時のスタックポインタの先頭の値を rbp として保存
    printf("  sub rsp, 208\n"); // rsp を変数が要求する分だけ押し下げる

    int line = 0;
    while (nodes[line] != NULL)
    {
        // AST -> ASM
        gen(nodes[line]);
        line++;

        // 式の評価値としてスタックに一つの値が残っているはずなので、
        // 溢れないように pop しておく
        printf("  pop rax\n");
    }

    // エピローグ
    // 最後の式の結果が rax に残っている
    printf("  mov rsp, rbp\n"); // スタックポインタを main 突入前に戻す
    printf("  pop rbp\n");      // rbp を main 突入前に戻す
    printf("  ret\n");          // rsp がリターンアドレスを指しているので、それを使って呼び出し箇所まで戻る
    return 0;
}