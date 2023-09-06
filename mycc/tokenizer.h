#pragma once

typedef enum
{
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
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
    int len;        // トークンの長さ
};

// トークン種別を文字列にします。
const char *tokenKindToString(TokenKind kind);

// 入力文字列 p をトークナイズします。
void tokenize(char *p);