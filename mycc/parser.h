#pragma once

// parser
typedef enum
{
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LE,  // <=
    ND_LT,  // <
    ND_GE,  // >=
    ND_GT,  // <
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整数
} NodeKind;

typedef struct Node Node;

// AST のノード
struct Node
{
    NodeKind kind; // 種類
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kind が ND_NUM の場合のみ
};

Node *parse();