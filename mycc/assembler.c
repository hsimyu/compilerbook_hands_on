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

int label_index = 0;

// assembler
void gen(Node *node)
{
    switch (node->kind)
    {
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;
    case ND_LVAR: // ローカル変数の参照: スタックトップに評価値を積む
        printf("# LVAR BEGIN\n");
        gen_lval(node);               // node が示す変数のアドレスをスタックに積む命令を生成
        printf("  pop rax\n");        // 変数アドレスを取り出す
        printf("  mov rax, [rax]\n"); // 変数アドレスに格納されている値を取り出す
        printf("  push rax\n");       // 取り出した値をスタックに push する
        printf("# LVAR END\n");
        return;
    case ND_ASSIGN:
        printf("# ASSIGN BEGIN\n");
        gen_lval(node->lhs);          // 左辺が示す変数のアドレスをスタックに積む
        gen(node->rhs);               // 右辺の評価値をスタックに積む
        printf("  pop rdi\n");        // 右辺の評価値を取り出す
        printf("  pop rax\n");        // 変数アドレスを取り出す
        printf("  mov [rax], rdi\n"); // 変数アドレスに評価値を格納
        printf("  push rdi\n");       // 代入式の評価値は代入結果とするので、rdi をスタックに積む
        printf("# ASSIGN END\n");
        return;
    case ND_RETURN:
        printf("# RETURN\n");
        gen(node->lhs);
        printf("  pop rax\n");      // 左辺の評価値を取り出す
        printf("  mov rsp, rbp\n"); // エピローグ
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    case ND_IF:
        printf("# IF\n");
        gen(node->lhs);                        // 条件式を評価
        printf("  pop rax\n");                 // 条件の評価値を取り出す
        printf("  cmp rax, 0\n");              // 条件の評価値が 0 かどうか
        printf("  je .Lend%d\n", label_index); // 評価値が 0 なら stmt を飛ばす
        gen(node->rhs);                        // if ブロックの内部を評価
        printf(".Lend%d:\n", label_index);     // ジャンプ用ラベル
        label_index++;
        return;
    case ND_IFELSE:
        printf("# IFELSE\n");
        gen(node->lhs);                         // 条件式を評価
        printf("  pop rax\n");                  // 条件の評価値を取り出す
        printf("  cmp rax, 0\n");               // 条件の評価値が 0 かどうか
        printf("  je .Lelse%d\n", label_index); // 評価値が 0 なら else へ飛ぶ
        gen(node->rhs);                         // if ブロックの内部を評価
        printf("  jmp .Lend%d\n", label_index); // end へ飛ぶ
        printf(".Lelse%d:\n", label_index);     // else ラベル
        gen(node->els);                         // else ブロックの内部を評価
        printf(".Lend%d:\n", label_index);      // end ラベル
        label_index++;
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind)
    {
    case ND_EQ:
        printf("# EQ\n");
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");       // al は rax の下位 8 ビット
        printf("  movzb rax, al\n"); // 残り 56 ビットをクリア
        break;
    case ND_NE:
        printf("# NE\n");
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LE:
        printf("# LE\n");
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("# LT\n");
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_GE:
        printf("# LT\n");
        // GE, GT は rax, rdi を入れ替える
        printf("  cmp rdi, rax\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_GT:
        printf("# GT\n");
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
