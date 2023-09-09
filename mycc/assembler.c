#include <stdio.h>

#include "util.h"
#include "parser.h"

void gen_lval(Node *node)
{
    if (node->kind != ND_LVAR)
        error("Failed to generate assembly: left-hand side of assign node is not Identifier: %c", node->kind);

    printf("  mov rax, rbp\n");              // rax に関数トップの値を入れて、
    printf("  sub rax, %d\n", node->offset); // 変数名に対応するオフセット値だけ rax を下げる
    printf("  push rax\n");                  // rax の値 (= 変数のアドレス) をスタックに push する
}

// assembler
void gen(Node *node)
{
    switch (node->kind)
    {
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;
    case ND_LVAR:                     // ローカル変数の参照: スタックトップに評価値を積む
        gen_lval(node);               // node が示す変数のアドレスをスタックに積む命令を生成
        printf("  pop rax\n");        // 変数アドレスを取り出す
        printf("  mov rax, [rax]\n"); // 変数アドレスに格納されている値を取り出す
        printf("  push rax\n");       // 取り出した値をスタックに push する
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);          // 左辺が示す変数のアドレスをスタックに積む
        gen(node->rhs);               // 右辺の評価値をスタックに積む
        printf("  pop rdi\n");        // 右辺の評価値を取り出す
        printf("  pop rax\n");        // 変数アドレスを取り出す
        printf("  mov [rax], rdi\n"); // 変数アドレスに評価値を格納
        printf("  push rdi\n");       // 代入式の評価値は代入結果とするので、rdi をスタックに積む
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("  pop rax\n");      // 左辺の評価値を取り出す
        printf("  mov rsp, rbp\n"); // エピローグ
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind)
    {
    case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");       // al は rax の下位 8 ビット
        printf("  movzb rax, al\n"); // 残り 56 ビットをクリア
        break;
    case ND_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_GE:
        // GE, GT は rax, rdi を入れ替える
        printf("  cmp rdi, rax\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_GT:
        printf("  cmp rdi, rax\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        // imul は rdx と rax を取って 128 ビットの値とし、
        // 引数のレジスタで割り、
        // 商を rax に、余りを rdx にセットする命令
        // cqo: rax に入っている 64bit の値を 128bit に伸ばして rdx と rax にセットする
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    default:
        error("Failed to generate assembly: unknown node = %d", node->kind);
        break;
    }

    printf("  push rax\n");
}
