#pragma once

#include "type.h"

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
    ND_NUM,      // 整数リテラル
    ND_STRING,   // 文字列リテラル
    ND_LVAR_REF, // ローカル変数参照
    ND_LVAR_DEC, // ローカル変数定義
    ND_RETURN,   // return
    ND_IF,       // if
    ND_IFELSE,   // if ... else
    ND_FOR,      // for
    ND_WHILE,    // while
    ND_BLOCK,    // { ... }
    ND_FUNCCALL, // call f()
    ND_FUNCDEF,  // def f()
    ND_GVAR_DEF, // グローバル変数定義
    ND_GVAR_REF, // グローバル変数参照
} NodeKind;

typedef struct StringLiteral StringLiteral;
struct StringLiteral
{
    char *val;           // 文字列
    int len;             // 長さ
    int asm_index;       // アセンブル時のインデックス
    StringLiteral *next; // 次
};

typedef struct LVar LVar;
struct LVar
{
    LVar *next; // 次の変数または NULL
    char *name; // 変数の名前
    int len;    // 名前の長さ
    Type *ty;   // 変数の型
    int offset; // RBP からのオフセット
};

typedef struct GVar GVar;
struct GVar
{
    GVar *next; // 次の変数または NULL
    char *name; // 変数の名前
    int len;    // 名前の長さ
    Type *ty;   // 変数の型
};

typedef struct Node Node;

// AST のノード
struct Node
{
    NodeKind kind;          // 種類
    Node *lhs;              // 左辺
    Node *rhs;              // 右辺
    int val_num;            // kind が ND_NUM の場合のみ
    StringLiteral *val_str; // kind が ND_STRING の場合のみ
    LVar *lvar_info;        // kind が ND_LVAR の場合のみ
    GVar *gvar_info;        // kind が ND_GVAR の場合のみ
    Node *opt_a;            // kind が ND_IFELSE, ND_FOR の場合のみ
    Node *opt_b;            // kind が ND_FOR の場合のみ
    Node *next;             // kind が ND_BLOCK の場合のみ
    char *fname;            // kind が ND_FUNCCALL の場合のみ
    int fname_len;          // kind が ND_FUNCCALL の場合のみ
    Node *call_args;        // kind が ND_FUNCCALL の場合のみ
    int arg_count;          // kind が ND_FUNCDEF の場合のみ
    LVar *locals;           // kind が ND_FUNCDEF の場合のみ: ローカル変数のリスト
    int locals_size;        // kind が ND_FUNCDEF の場合のみ: ローカル変数の合計サイズ
};

// トークン列をパースします。
Node **parse();

// パース結果として定義されたグローバル変数のリストを返します。
GVar *get_gvar();

// パース結果として定義された文字列リテラルのリストを返します。
StringLiteral *get_string_literals();