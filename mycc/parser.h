#pragma once

// parser
typedef enum
{
    ND_ASSIGN,   // =
    ND_EQ,       // ==
    ND_NE,       // !=
    ND_LE,       // <=
    ND_LT,       // <
    ND_GE,       // >=
    ND_GT,       // <
    ND_ADD,      // +
    ND_ADDPTR,   // ポインタに対する +
    ND_SUB,      // -
    ND_SUBPTR,   // ポインタに対する -
    ND_MUL,      // 二項*
    ND_DIV,      // /
    ND_DEREF,    // 単項*
    ND_ADDR,     // 単項&
    ND_NUM,      // 整数
    ND_LVAR_REF, // ローカル変数
    ND_LVAR_DEC, // ローカル変数
    ND_RETURN,   // return
    ND_IF,       // if
    ND_IFELSE,   // if ... else
    ND_FOR,      // for
    ND_WHILE,    // while
    ND_BLOCK,    // { ... }
    ND_FUNCCALL, // call f()
    ND_FUNCDEF,  // def f()
} NodeKind;

typedef enum
{
    TYPE_INT,
    TYPE_PTR,
    TYPE_ARRAY,
    TYPE_INVALID,
} TypeKind;

typedef struct Type Type;
struct Type
{
    TypeKind kind;     // 型の種類
    Type *ptr_to;      // PTR: ポインタ型の指し先の型
    int ptr_depth;     // PTR: ポインタの深度 (* なら 1, ** なら 2), ptr_to をたどっていくようにすれば定義不要
    size_t array_size; // ARRAY: 固定長配列サイズ
};

typedef struct LVar LVar;
struct LVar
{
    LVar *next; // 次の変数または NULL
    char *name; // 変数の名前
    Type *ty;   // 変数の型
    int len;    // 長さ
    int offset; // RBP からのオフセット
};

typedef struct Node Node;

// AST のノード
struct Node
{
    NodeKind kind;   // 種類
    Node *lhs;       // 左辺
    Node *rhs;       // 右辺
    int val;         // kind が ND_NUM の場合のみ
    LVar *var_info;  // kind が ND_LVAR の場合のみ
    Node *opt_a;     // kind が ND_IFELSE, ND_FOR の場合のみ
    Node *opt_b;     // kind が ND_FOR の場合のみ
    Node *next;      // kind が ND_BLOCK の場合のみ
    char *fname;     // kind が ND_FUNCCALL の場合のみ
    int fname_len;   // kind が ND_FUNCCALL の場合のみ
    Node *call_args; // kind が ND_FUNCCALL の場合のみ
    int arg_count;   // kind が ND_FUNCDEF の場合のみ
    LVar *locals;    // kind が ND_FUNCDEF の場合のみ: ローカル変数のリスト
    int locals_size; // kind が ND_FUNCDEF の場合のみ: ローカル変数の合計サイズ
};

// トークン列をパースします。
Node **parse();
