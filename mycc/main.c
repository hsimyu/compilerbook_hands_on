#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    TK_RESERVED, // 記号
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わり
} TokenKind;

typedef struct Token Token;

struct Token
{
    TokenKind kind; // トークン型
    Token *next;    // 次の入力トークン
    int val;        // TK_NUM の場合、その数値
    char *str;      // トークン文字列
};

// 現在着目しているトークン
Token *current_token;

// エラーを出力します。
void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて true を返す。
// それ以外の場合には false を返す
bool consume(char op)
{
    if (current_token->kind != TK_RESERVED || current_token->str[0] != op)
        return false;

    current_token = current_token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める
// それ以外の場合にはエラーを報告する。
void expect(char op)
{
    if (current_token->kind != TK_RESERVED || current_token->str[0] != op)
        error("Invalid TokenKind: expected = '%c', actual = '%c", op, current_token->kind);

    current_token = current_token->next;
}

// 次のトークンが数値の場合、トークンを 1 つ読み進めてその数値を返す。
// それ以外の場合には、エラーを報告する。
int expect_number()
{
    if (current_token->kind != TK_NUM)
        error("Invalid TokenKind: excected = Number, actual = '%c'", current_token->kind);

    int val = current_token->val;
    current_token = current_token->next;
    return val;
}

// 入力の終わりかどうかを判定します。
bool at_eof()
{
    return current_token->kind == TK_EOF;
}

// 新しいトークンを作成して、cur に繋げます。
Token *new_token(TokenKind kind, Token *cur, char *str)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

// 入力文字列 p をトークナイズして、それを返します。
Token *tokenize(char *p)
{
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p)
    {
        // 空白文字 -> スキップ
        if (isspace(*p))
        {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-')
        {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
            ;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("Failed to tokenize: %c", *p);
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Invalid argument count: arg must be 1.\n");
        return 1;
    }

    // トークン列を生成
    current_token = tokenize(argv[1]);

    // アセンブリのヘッダー
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // 式の最初は数字であるという制約を満たしているか確認
    printf("  mov rax, %d\n", expect_number());

    while (!at_eof())
    {
        if (consume('+'))
        {
            // + の後には数字が来ないといけない
            // 次の演算子まで消費
            printf("  add rax, %d\n", expect_number());
            continue;
        }

        // それ以外は - である
        expect('-');

        // 次の演算子まで消費
        printf("  sub rax, %d\n", expect_number());
    }

    printf("  ret\n");
    return 0;
}