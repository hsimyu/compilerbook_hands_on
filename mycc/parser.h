#pragma once

// parser
typedef enum
{
    ND_ASSIGN, // =
    ND_EQ,     // ==
    ND_NE,     // !=
    ND_LE,     // <=
    ND_LT,     // <
    ND_GE,     // >=
    ND_GT,     // <
    ND_ADD,    // +
    ND_SUB,    // -
    ND_MUL,    // *
    ND_DIV,    // /
    ND_NUM,    // 整数
    ND_LVAR,   // ローカル変数
    ND_RETURN, // return
    ND_IF,     // if
    ND_IFELSE, // if ... else
    ND_FOR,    // for
    ND_WHILE,  // while
} NodeKind;

typedef struct Node Node;

// AST のノード
struct Node
{
    NodeKind kind; // 種類
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kind が ND_NUM の場合のみ
    int offset;    // kind が ND_LVAR の場合のみ: ローカル変数のベースポインタからのオフセット値
    Node *opt_a;   // kind が ND_IFELSE, ND_FOR の場合のみ
    Node *opt_b;   // kind が ND_FOR の場合のみ
};

// トークン列をパースします。
Node **parse();
