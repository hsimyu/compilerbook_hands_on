#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "tokenizer.h"

// 現在着目しているトークン
Token *token;

const char *tokenKindToString(TokenKind kind)
{
    switch (kind)
    {
    case TK_RESERVED:
        return "Reserved";
    case TK_IDENT:
        return "Identifier";
    case TK_NUM:
        return "Number";
    case TK_EOF:
        return "EOF";
    }

    return "Unknown";
}

// 入力の終わりかどうかを判定します。
bool at_eof()
{
    return token->kind == TK_EOF;
}

// 次のトークンの文字列長を調べます。
int count_token_length(char *str)
{
    if (strncmp(str, "==", 2) == 0 ||
        strncmp(str, "!=", 2) == 0 ||
        strncmp(str, ">=", 2) == 0 ||
        strncmp(str, "<=", 2) == 0)
    {
        return 2;
    }

    return 1;
}

// 新しいトークンを作成して、cur に繋げます。
Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    tok->str = str;
    cur->next = tok;
    return tok;
}

// 入力文字列 p をトークナイズします。
void tokenize(char *p)
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

        if ('a' <= *p && *p <= 'z')
        {
            // 識別子は 1 文字固定
            cur = new_token(TK_IDENT, cur, p++, 1);
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '=' || *p == '!' || *p == '<' || *p == '>')
        {
            int token_length = count_token_length(p);
            cur = new_token(TK_RESERVED, cur, p, token_length);
            p = p + token_length;
            continue;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p, 0); // len は dummy
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(cur->str + 1, "Failed to tokenize: %c", *p);
    }

    new_token(TK_EOF, cur, p, 1);
    token = head.next;
}
