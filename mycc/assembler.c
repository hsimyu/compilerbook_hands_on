#include <stdio.h>

#include "util.h"
#include "parser.h"
#include "type.h"

void gen_string_literals()
{
    StringLiteral *str_literals = get_string_literals();
    int literal_index = 0;

    // TODO: ここでグローバル変数の定義も吐き出すようにする
    while (str_literals != NULL)
    {
        printf(".LC%d:\n", literal_index);
        printf("  .string \"%.*s\"\n", str_literals->len, str_literals->val);
        str_literals->asm_index = literal_index;
        str_literals = str_literals->next;
        literal_index++;
    }
}

void gen_lval(Node *node);
void gen_rval(Node *node);
void gen(Node *node);

// 左辺値の評価: 変数参照が指しているアドレスを評価
void gen_lval(Node *node)
{
    if (node->kind != ND_LVAR_REF && node->kind != ND_GVAR_REF && node->kind != ND_DEREF)
        error("Failed to generate assembly: left-hand side of assign node is not Variable Reference or Dereference: %c", node->kind);

    if (node->kind == ND_DEREF)
    {
        printf("# DEREF (lhs) BEGIN\n");
        // 左辺値としての変数参照にデリファレンスが指定されているので、
        // lhs に格納されている値を右辺値として評価して返す
        // lhs にはアドレス値が入っているので、その評価値はアドレスになる
        gen(node->lhs);
        printf("# DEREF END\n");
    }
    else if (node->kind == ND_GVAR_REF)
    {
        // 左辺値としてのグローバル変数参照なので、アドレス値をスタックに積んで返す
        printf("  lea rax, %.*s[rip]\n", node->gvar_info->len, node->gvar_info->name);
        printf("  push rax\n");
    }
    else
    {
        // 左辺値としてのローカル変数参照なので、アドレスを積んで返す
        printf("  mov rax, rbp\n");                         // rax に関数トップの値を入れて、
        printf("  sub rax, %d\n", node->lvar_info->offset); // 変数名に対応するオフセット値だけ rax を下げる
        printf("  push rax\n");                             // rax の値 (= 変数のアドレス) をスタックに push する
    }
}

void evaluate_rval(Node *node)
{
    // 前提: スタックトップに評価対象のアドレスが乗っている
    printf("  pop rax\n"); // 変数アドレスを取り出す

    // 変数アドレスに格納されている値を取り出す
    // 変数サイズによって取り出し方を変える
    if (is_char(node))
    {
        // TODO: char のポインタのことを考えてない
        printf("  movsx eax, BYTE PTR [rax]\n");
    }
    else if (is_address(node))
    {
        // int*
        printf("  mov rax, QWORD PTR [rax]\n");
    }
    else
    {
        // int
        printf("  mov eax, DWORD PTR [rax]\n");
    }

    printf("  push rax\n"); // 取り出した値をスタックに push する
}

// 右辺値の評価: 変数参照が指しているアドレスに格納されている値を評価
void gen_rval(Node *node)
{
    if (is_array(node))
    {
        // 配列ノードを右辺値評価した場合、変数のアドレスをスタックに積むのみでよい
        // なぜなら *a = 1 のような文の時、書き込み先は a + 0 であり、a のアドレスが指している箇所に書き込めばよいから
        // a の位置にはアドレス値は入っていないため [rax] のように評価してしまうと不正になる
        gen_lval(node);
    }
    else if (node->kind == ND_ADDPTR || node->kind == ND_SUBPTR)
    {
        // ここに到達するのは、右辺値としてのデリファレンス時
        // ノードの計算結果がアドレスとして積まれる
        gen(node);
    }
    else
    {
        gen_lval(node); // node が示す変数のアドレスをスタックに積む命令を生成
        evaluate_rval(node);
    }
}

int label_index = 0;
int acquire_label_index()
{
    return label_index++;
}

Node *gen_block(Node *node)
{
    if (node->kind != ND_BLOCK)
        error("Failed to generate assembly: non-block node processed as block: kind = %c", node->kind);

    printf("# {\n");
    Node *lastEvaluated = node;
    while (lastEvaluated->next != NULL)
    {
        gen(lastEvaluated->next);
        lastEvaluated = lastEvaluated->next;

        if (lastEvaluated->next != NULL &&
            lastEvaluated->kind != ND_WHILE &&
            lastEvaluated->kind != ND_FOR &&
            lastEvaluated->kind != ND_IF &&
            lastEvaluated->kind != ND_IFELSE &&
            lastEvaluated->kind != ND_LVAR_DEC)
        {
            // 最後の一つの文以外の評価値は不要
            // スタックに積んであるので捨てる
            // 制御構文の場合はスタックに評価値がないので不要
            printf("  pop rax\n");
        }
    }
    printf("# }\n");
    return lastEvaluated;
}

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
        printf("# -- FUNC BEGIN: %.*s --\n", node->fname_len, node->fname);
        // TODO: .globl について調べる
        printf(".globl %.*s\n", node->fname_len, node->fname);
        printf("%.*s:\n", node->fname_len, node->fname); // 関数名ラベル

        // プロローグ
        printf("  push rbp\n");     // 関数呼び出し時の rbp をスタックに保存
        printf("  mov rbp, rsp\n"); // rbp に現在のスタックトップのアドレスを保存
        // 必要なローカル変数の分だけスタックを確保 (仮引数分もローカル変数に含まれている)
        if (node->locals_size > 0)
        {
            // x86-64 ABI のため、16byte アラインメントを取っておく
            int misaligned = node->locals_size % 16;
            if (misaligned != 0)
            {
                printf("  sub rsp, %d\n", node->locals_size + (16 - misaligned));
            }
            else
            {
                printf("  sub rsp, %d\n", node->locals_size);
            }
        }

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
            // TODO: 引数のサイズによって取り出し方を変える
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
        Node *lastEvaluated = gen_block(node->lhs);

        // エピローグ
        // ブロック内最後の評価ノードが ND_RETURN であれば不要
        if (lastEvaluated->kind != ND_RETURN)
        {
            printf("  pop rax\n");      // スタックトップの評価値を rax へ取り出す
            printf("  mov rsp, rbp\n"); // rsp を rbp まで戻す
            printf("  pop rbp\n");      // 前の rbp を復元
            printf("  ret\n");          // リターンアドレスへ戻る
        }

        printf("# -- FUNC END: %.*s --\n\n", node->fname_len, node->fname);
        return;
    case ND_NUM:
        printf("# NUM\n");
        printf("  push %d\n", node->val_num);
        return;
    case ND_STRING:
        // 変数に入れない文字列リテラルの場合はここに到達する
        // リテラルのアドレスを (ポインタとして) 積む
        printf("# STRING\n");
        printf("  push OFFSET FLAT:.LC%d\n", node->val_str->asm_index);
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
        gen_rval(node->lhs); // lhs の指し先の値がスタックに積まれている
        evaluate_rval(node->lhs);
        printf("# DEREF END\n");
        return;
    case ND_LVAR_DEC: // ローカル変数の宣言
        return;
    case ND_LVAR_REF: // ローカル変数の参照: スタックトップに評価値を積む
        printf("# LVAR BEGIN\n");
        gen_rval(node);
        printf("# LVAR END\n");
        return;
    case ND_GVAR_DEF: // グローバル変数の定義
        printf("# GVAR DEF\n");
        printf(".data\n");
        printf(".globl %.*s\n", node->gvar_info->len, node->gvar_info->name);
        printf("%.*s:\n", node->gvar_info->len, node->gvar_info->name);
        printf("  .zero %u\n", node->gvar_info->ty->type_size);
        printf("\n");
        return;
    case ND_GVAR_REF: // グローバル変数の参照
        // グローバル変数のアドレスに格納されている値を push する
        printf("# GVAR REF: %.*s\n", node->gvar_info->len, node->gvar_info->name);
        // TODO: なんで [rip] って書かなくちゃいけないのかよく分かってない
        printf("  push %.*s[rip]\n", node->gvar_info->len, node->gvar_info->name);
        return;
    case ND_ASSIGN:
        printf("# ASSIGN BEGIN\n");
        if (node->rhs != NULL && node->rhs->kind == ND_STRING)
        {
            gen_lval(node->lhs);   // 左辺が示すアドレスをスタックに積む
            printf("  pop rax\n"); // 変数アドレスを取り出す

            printf("# STRING LITERAL\n");
            // 右辺が文字列リテラルの時はレジスタを介さないで、テキストラベルを参照する
            // TODO: OFFSET FLAT って何
            printf("  mov QWORD PTR [rax], OFFSET FLAT:.LC%d\n", node->rhs->val_str->asm_index);
            printf("  push 1\n"); // 適当な値をスタックに積む?
        }
        else
        {
            gen_lval(node->lhs);   // 左辺が示すアドレスをスタックに積む
            gen(node->rhs);        // 右辺の評価値をスタックに積む
            printf("  pop rdi\n"); // 右辺の評価値を取り出す
            printf("  pop rax\n"); // 変数アドレスを取り出す

            if (is_char(node->lhs))
            {
                printf("  mov BYTE PTR [rax], dil\n"); // 8bit レジスタを使って、変数アドレスに評価値を格納
            }
            else if (is_address(node->lhs))
            {
                printf("  mov QWORD PTR [rax], rdi\n"); // 変数アドレスに評価値を格納
            }
            else
            {
                printf("  mov DWORD PTR [rax], edi\n"); // 変数アドレスに評価値を格納
            }

            printf("  push rdi\n"); // 代入式の評価値は代入結果とするので、rdi をスタックに積む
        }
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
    {
        printf("# IF\n");
        gen(node->lhs);           // 条件式を評価
        printf("  pop rax\n");    // 条件の評価値を取り出す
        printf("  cmp rax, 0\n"); // 条件の評価値が 0 かどうか
        int li = acquire_label_index();
        printf("  je .Lend%d\n", li); // 評価値が 0 なら stmt を飛ばす
        gen(node->rhs);               // if ブロックの内部を評価
        printf(".Lend%d:\n", li);     // ジャンプ用ラベル
        printf("# IF END\n");
        return;
    }
    case ND_IFELSE:
    {
        printf("# IFELSE\n");
        gen(node->lhs);           // 条件式を評価
        printf("  pop rax\n");    // 条件の評価値を取り出す
        printf("  cmp rax, 0\n"); // 条件の評価値が 0 かどうか
        int li = acquire_label_index();
        printf("  je .Lelse%d\n", li); // 評価値が 0 なら else へ飛ぶ
        gen(node->rhs);                // if ブロックの内部を評価
        printf("  jmp .Lend%d\n", li); // end へ飛ぶ
        printf(".Lelse%d:\n", li);     // else ラベル
        gen(node->opt_a);              // else ブロックの内部を評価
        printf(".Lend%d:\n", li);      // end ラベル
        printf("# IFELSE END\n");
        return;
    }
    case ND_WHILE:
    {
        printf("# WHILE\n");
        int li = acquire_label_index();
        printf(".Lbegin%d:\n", li);      // begin ラベル
        gen(node->lhs);                  // 条件式を評価
        printf("  pop rax\n");           // 条件の評価値を取り出す
        printf("  cmp rax, 0\n");        // 条件の評価値が 0 かどうか
        printf("  je .Lend%d\n", li);    // 0 なら end へ飛んで終了
        gen(node->rhs);                  // while ブロックの内部を評価
        printf("  pop rax\n");           // rhs の評価値がスタックに積んであるので捨てる
        printf("  jmp .Lbegin%d\n", li); // begin へ飛んでやり直し
        printf(".Lend%d:\n", li);        // end ラベル
        printf("# WHILE END\n");
        return;
    }
    case ND_FOR:
    {
        printf("# FOR\n");
        int li = acquire_label_index();
        gen(node->lhs);                  // for (A; B; C) の A を評価 (初期化)
        printf(".Lbegin%d:\n", li);      // begin ラベル
        gen(node->opt_a);                // B を実行
        printf("  pop rax\n");           // B の評価値を取り出す
        printf("  cmp rax, 0\n");        // B の評価値が 0 かどうか
        printf("  je .Lend%d\n", li);    // 0 なら end へ飛んで終了
        gen(node->rhs);                  // for ブロックの内部を評価
        printf("  pop rax\n");           // stmt の評価値がスタックに積んであるので捨てる
        gen(node->opt_b);                // C を評価
        printf("  pop rax\n");           // C の評価値がスタックに積んであるので捨てる
        printf("  jmp .Lbegin%d\n", li); // begin へ飛んでやり直し
        printf(".Lend%d:\n", li);        // end ラベル
        printf("# FOR END\n");
        return;
    }
    case ND_BLOCK:
    {
        gen_block(node);
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

        printf("  mov al, 0\n"); // 浮動小数点の引数の個数は 0
        printf("  call %.*s\n", node->fname_len, node->fname);
        printf("  push rax\n"); // 戻り値をスタックに積む
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
