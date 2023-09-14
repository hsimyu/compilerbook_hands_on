#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
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
    case TK_RETURN:
        return "return";
    case TK_IF:
        return "if";
    case TK_ELSE:
        return "else";
    case TK_WHILE:
        return "while";
    case TK_FOR:
        return "for";
    case TK_EOF:
        return "EOF";
    default:
        return "Unknown";
    }
}

// 文字が英数字またはアンダースコアかどうかを判定します。
int is_alphabet_or_number(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == '_');
}

// 次の識別子トークンの文字列長を調べます。
int count_identifier_length(char *str)
{
    int length = 0;
    char c = str[length];
    while (is_alphabet_or_number(c))
    {
        length++;
        c = str[length];
    }
    return length;
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

        if (strncmp(p, "return", 6) == 0 && !is_alphabet_or_number(p[6]))
        {
            cur = new_token(TK_RETURN, cur, p, 6);
            p = p + 6;
            continue;
        }

        if (strncmp(p, "if", 2) == 0 && !is_alphabet_or_number(p[2]))
        {
            cur = new_token(TK_IF, cur, p, 2);
            p = p + 2;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !is_alphabet_or_number(p[4]))
        {
            cur = new_token(TK_ELSE, cur, p, 4);
            p = p + 4;
            continue;
        }

        if (strncmp(p, "while", 5) == 0 && !is_alphabet_or_number(p[5]))
        {
            cur = new_token(TK_WHILE, cur, p, 5);
            p = p + 5;
            continue;
        }

        if (strncmp(p, "for", 3) == 0 && !is_alphabet_or_number(p[3]))
        {
            cur = new_token(TK_FOR, cur, p, 3);
            p = p + 3;
            continue;
        }

        if ('a' <= *p && *p <= 'z')
        {
            int token_length = count_identifier_length(p);
            cur = new_token(TK_IDENT, cur, p, token_length);
            p = p + token_length;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '=' || *p == '!' || *p == '<' || *p == '>' || *p == ';' || *p == '{' || *p == '}' || *p == ',')
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
