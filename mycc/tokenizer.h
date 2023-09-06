#pragma once

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
    int len;        // トークンの長さ
};

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて true を返す。
// それ以外の場合には false を返す
bool consume(char *op);

// 次のトークンが期待している記号のときには、トークンを1つ読み進める
// それ以外の場合にはエラーを報告する。
void expect(char *op);

// 次のトークンが数値の場合、トークンを 1 つ読み進めてその数値を返す。
// それ以外の場合には、エラーを報告する。
int expect_number();

// 入力文字列 p をトークナイズします。
void tokenize(char *p);