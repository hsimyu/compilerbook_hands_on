#include <stdio.h>

#include "util.h"
#include "parser.h"

// assembler
void gen(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("  push %d\n", node->val);
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
