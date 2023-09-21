#include <stdio.h>

#include "util.h"
#include "parser.h"

void gen_rval(Node *node);

// 左辺値の評価: 変数参照が指しているアドレスを評価
void gen_lval(Node *node)
{
    if (node->kind != ND_LVAR_REF && node->kind != ND_DEREF)
        error("Failed to generate assembly: left-hand side of assign node is not Identifier or Dereference: %c", node->kind);

    if (node->kind == ND_DEREF)
    {
        printf("# DEREF (lhs) BEGIN\n");
        // 左辺値としての変数参照にデリファレンスが指定されているので、
        // lhs に格納されている値を右辺値として評価して返す
        // lhs にはアドレス値が入っているので、その評価値はアドレスになる
        gen_rval(node->lhs);
        printf("# DEREF END\n");
    }
    else
    {
        // 左辺値としての変数参照なので、アドレスを積んで返す
        printf("  mov rax, rbp\n");                        // rax に関数トップの値を入れて、
        printf("  sub rax, %d\n", node->var_info->offset); // 変数名に対応するオフセット値だけ rax を下げる
        printf("  push rax\n");                            // rax の値 (= 変数のアドレス) をスタックに push する
    }
}

// 右辺値の評価: 変数参照が指しているアドレスに格納されている値を評価
void gen_rval(Node *node)
{
    gen_lval(node);               // node が示す変数のアドレスをスタックに積む命令を生成
    printf("  pop rax\n");        // 変数アドレスを取り出す
    printf("  mov rax, [rax]\n"); // 変数アドレスに格納されている値を取り出す
    printf("  push rax\n");       // 取り出した値をスタックに push する
}

int label_index = 0;

// assembler
void gen(Node *node)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->kind)
    {
    case ND_FUNCDEF:
        printf("# FUNCDEF\n");
        printf("%.*s:\n", node->fname_len, node->fname); // 関数名ラベル

        // プロローグ
        printf("  push rbp\n");                       // 関数呼び出し時の rbp をスタックに保存
        printf("  mov rbp, rsp\n");                   // rbp に現在のスタックトップのアドレスを保存
        printf("  sub rsp, %d\n", node->locals_size); // 必要なローカル変数の分だけスタックを確保 (仮引数分もローカル変数に含まれている)

        // レジスタからスタックへ仮引数を割り当てる
        // 1: rdi
        // 2: rsi
        // 3: rdx
        // 4: rcx
        // 5: r8
        // 6: r9
        // 以降: スタック
        int arg_count = 0;
        Node *arg = node->next;
        while (arg != NULL)
        {
            // int 以外の引数を想定していない
            arg_count++;
            switch (arg_count)
            {
            case 1:
                printf("  mov [rbp-8], rdi\n");
                break;
            case 2:
                printf("  mov [rbp-16], rsi\n");
                break;
            case 3:
                printf("  mov [rbp-24], rdx\n");
                break;
            case 4:
                printf("  mov [rbp-32], rcx\n");
                break;
            case 5:
                printf("  mov [rbp-40], r8\n");
                break;
            case 6:
                printf("  mov [rbp-48], r9\n");
                break;
            default:
                // 7 以上はスタックから取り出す
                printf("  pop rax\n");
                printf("  mov [rbp-%d], rax\n", arg_count * 8);
                break;
            }
            arg = arg->next;
        }

        // ブロックを評価
        gen(node->lhs);

        // エピローグ
        printf("  pop rax\n");      // スタックトップの評価値を rax へ取り出す
        printf("  mov rsp, rbp\n"); // rsp を rbp まで戻す
        printf("  pop rbp\n");      // 前の rbp を復元
        printf("  ret\n");          // リターンアドレスへ戻る
        printf("# FUNCDEF END\n");
        return;
    case ND_NUM:
        printf("# NUM\n");
        printf("  push %d\n", node->val);
        return;
    case ND_ADDR:
        printf("# ADDR\n");
        gen_lval(node->lhs); // lhs が示す変数のアドレスをスタックに積む
        printf("# ADDR END\n");
        return;
    case ND_DEREF:
        // ここに到達するのは右辺値としてのデリファレンス
        // (左辺値として代入対象になる時、ND_ASSIGN から gen_lval に飛ぶ)
        printf("# DEREF (rhs) BEGIN\n");
        gen_rval(node->lhs);          // lhs の指し先の値がスタックに積まれている
        printf("  pop rax\n");        // ポインタの指し先アドレスを取り出す
        printf("  mov rax, [rax]\n"); // 指し先に格納されている値を取り出す
        printf("  push rax\n");       // 取り出した値をスタックに push する
        printf("# DEREF END\n");
        return;
    case ND_LVAR_DEC: // ローカル変数の宣言
        return;
    case ND_LVAR_REF: // ローカル変数の参照: スタックトップに評価値を積む
        printf("# LVAR BEGIN\n");
        gen_rval(node);
        printf("# LVAR END\n");
        return;
    case ND_ASSIGN:
        printf("# ASSIGN BEGIN\n");
        gen_lval(node->lhs);          // 左辺が示すアドレスをスタックに積む
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
        printf("# IF END\n");
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
        gen(node->opt_a);                       // else ブロックの内部を評価
        printf(".Lend%d:\n", label_index);      // end ラベル
        printf("# IFELSE END\n");
        label_index++;
        return;
    case ND_WHILE:
        printf("# WHILE\n");
        printf(".Lbegin%d:\n", label_index);      // begin ラベル
        gen(node->lhs);                           // 条件式を評価
        printf("  pop rax\n");                    // 条件の評価値を取り出す
        printf("  cmp rax, 0\n");                 // 条件の評価値が 0 かどうか
        printf("  je .Lend%d\n", label_index);    // 0 なら end へ飛んで終了
        gen(node->rhs);                           // while ブロックの内部を評価
        printf("  pop rax\n");                    // rhs の評価値がスタックに積んであるので捨てる
        printf("  jmp .Lbegin%d\n", label_index); // begin へ飛んでやり直し
        printf(".Lend%d:\n", label_index);        // end ラベル
        label_index++;
        printf("# WHILE END\n");
        return;
    case ND_FOR:
        printf("# FOR\n");
        gen(node->lhs);                           // for (A; B; C) の A を評価 (初期化)
        printf(".Lbegin%d:\n", label_index);      // begin ラベル
        gen(node->opt_a);                         // B を実行
        printf("  pop rax\n");                    // B の評価値を取り出す
        printf("  cmp rax, 0\n");                 // B の評価値が 0 かどうか
        printf("  je .Lend%d\n", label_index);    // 0 なら end へ飛んで終了
        gen(node->rhs);                           // for ブロックの内部を評価
        printf("  pop rax\n");                    // stmt の評価値がスタックに積んであるので捨てる
        gen(node->opt_b);                         // C を評価
        printf("  pop rax\n");                    // C の評価値がスタックに積んであるので捨てる
        printf("  jmp .Lbegin%d\n", label_index); // begin へ飛んでやり直し
        printf(".Lend%d:\n", label_index);        // end ラベル
        printf("# FOR END\n");
        label_index++;
        return;
    case ND_BLOCK:
    {
        printf("# {\n");
        Node *target = node;
        while (target->next != NULL)
        {
            gen(target->next);
            target = target->next;

            if (target->next != NULL &&
                target->kind != ND_WHILE &&
                target->kind != ND_FOR &&
                target->kind != ND_IF &&
                target->kind != ND_IFELSE &&
                target->kind != ND_LVAR_DEC)
            {
                // 最後の一つの文以外の評価値は不要
                // スタックに積んであるので捨てる
                // 制御構文の場合はスタックに評価値がないので不要
                printf("  pop rax\n");
            }
        }
        printf("# }\n");
        return;
    }
    case ND_FUNCCALL:
    {
        printf("# FUNCCALL\n");

        // 引数をスタックに積む
        Node *target = node->call_args;

        int args_count = 0;
        while (target != NULL)
        {
            // この実装だと、左から順に評価することになる
            // 右から順に実行してスタックに乗せた方が一時退避が不要なので望ましい
            gen(target);
            target = target->call_args;
            args_count++;
        }

        // 引数を呼び出しレジスタに格納 (浮動小数点数の時は使用レジスタが異なる)
        // 1: rdi
        // 2: rsi
        // 3: rdx
        // 4: rcx
        // 5: r8
        // 6: r9
        // 以降: スタック
        switch (args_count)
        {
        case 6:
            printf("  pop r9\n");
            // fallthrough
        case 5:
            printf("  pop r8\n");
            // fallthrough
        case 4:
            printf("  pop rcx\n");
            // fallthrough
        case 3:
            printf("  pop rdx\n");
            // fallthrough
        case 2:
            printf("  pop rsi\n");
            // fallthrough
        case 1:
            printf("  pop rdi\n");
            break;
        default:
            break;
        }

        // x86-64 ABI:
        // 関数呼び出し時に、rsp が 16 の倍数、つまり下位 4 ビットが 0000 である必要がある
        printf("  mov rax, rsp\n");
        printf("  and rax, 15\n"); // rax = rax & 0b00001111, ビットマスク
        printf("  cmp rax, 0\n");  // ビットマスク結果が 0 なら rsp は 16 の倍数、そうでなければ 8 の倍数
        printf("  je .Lcallb%d\n", label_index);

        // rsp が 16 の倍数でない場合は呼び出し前後で rsp を調整する
        printf(".Lcalla%d:\n", label_index);
        printf("  sub rsp, 8\n");
        printf("  call %.*s\n", node->fname_len, node->fname);
        printf("  add rsp, 8\n");
        printf("  jmp .Lcallend%d\n", label_index);

        // rsp が 16 の倍数の場合は、そのまま呼び出せる
        printf(".Lcallb%d:\n", label_index);
        // 関数をコール
        printf("  call %.*s\n", node->fname_len, node->fname);
        printf(".Lcallend%d:\n", label_index); // 終了ラベル
        printf("  push rax\n");                // 戻り値をスタックに積む
        label_index++;
        return;
    }
    default:
        break;
    }

    printf("# BINARY OP\n");
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
        printf("# GE\n");
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
        printf("# ADD\n");
        printf("  add rax, rdi\n");
        break;
    case ND_ADDPTR:
        printf("# ADDPTR\n");
        // ポインタに対する加算なので、rdi の値をポインタが指す型のサイズだけ倍する
        // 左辺が int* -> sizeof(int): 4
        // 左辺が int** -> sizeof(int*): 8
        printf("  imul rdi, 4\n");
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("# SUB\n");
        printf("  sub rax, rdi\n");
        break;
    case ND_SUBPTR:
        printf("# SUBPTR\n");
        // ポインタに対する減算なので、rdi の値をポインタが指す型のサイズだけ倍する
        printf("  imul rdi, 4\n");
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("# MUL\n");
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("# DIV\n");
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
    printf("# BINARY OP END\n");
}
