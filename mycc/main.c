#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 入力プログラム
char *user_input;

// エラー箇所を報告します。
void error_at(char *loc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " "); // pos 個の空白
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラーを出力します。
void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

typedef enum
{
    TK_RESERVED, // 記号
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わり
} TokenKind;

typedef struct Token Token;

const char *tokenKindToString(TokenKind kind)
{
    switch (kind)
    {
    case TK_RESERVED:
        return "Reserved";
    case TK_NUM:
        return "Number";
    case TK_EOF:
        return "EOF";
    }

    return "Unknown";
}

struct Token
{
    TokenKind kind; // トークン型
    Token *next;    // 次の入力トークン
    int val;        // TK_NUM の場合、その数値
    char *str;      // トークン文字列
};

// 現在着目しているトークン
Token *token;

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて true を返す。
// それ以外の場合には false を返す
bool consume(char op)
{
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;

    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める
// それ以外の場合にはエラーを報告する。
void expect(char op)
{
    if (token->kind != TK_RESERVED || token->str[0] != op)
        error_at(token->str, "Invalid TokenKind: expected = '%s', actual = '%s",
                 tokenKindToString(op),
                 tokenKindToString(token->kind));

    token = token->next;
}

// 次のトークンが数値の場合、トークンを 1 つ読み進めてその数値を返す。
// それ以外の場合には、エラーを報告する。
int expect_number()
{
    if (token->kind != TK_NUM)
        error_at(token->str, "Invalid TokenKind: excected = 'Number', actual = '%s'",
                 tokenKindToString(token->kind));

    int val = token->val;
    token = token->next;
    return val;
}

// 入力の終わりかどうかを判定します。
bool at_eof()
{
    return token->kind == TK_EOF;
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
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(cur->str + 1, "Failed to tokenize: %c", *p);
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
    user_input = argv[1];
    token = tokenize(argv[1]);

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