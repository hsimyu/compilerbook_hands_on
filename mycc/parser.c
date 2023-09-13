#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "tokenizer.h"
#include "parser.h"

extern Token *token;

typedef struct LVar LVar;
struct LVar
{
    LVar *next; // 次の変数または NULL
    char *name; // 変数の名前
    int len;    // 長さ
    int offset; // RBP からのオフセット
};

// ローカル変数のリスト
LVar *locals;

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて true を返す。
// それ以外の場合には false を返す
bool consume_reserved(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;

    token = token->next;
    return true;
}

// 次のトークンが識別子の場合は、トークンを 1 つ読み進めてそのトークンを返す。
// それ以外の場合には NULL を返す。
Token *consume_ident()
{
    if (token->kind != TK_IDENT)
        return NULL;

    Token *result = token;
    token = token->next;
    return result;
}

// 次のトークンが期待したトークンの場合は、トークンを 1 つ読み進めて true を返す。
// それ以外の場合には false を返す。
bool consume_control(TokenKind kind)
{
    if (token->kind != kind)
        return false;

    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める
// それ以外の場合にはエラーを報告する。
void expect(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        error_at(token->str, "Invalid TokenKind: expected = '%s', actual = '%s",
                 op,
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

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// 変数を名前引きする
// 見つからなかったら NULL を返します。
LVar *find_lvar(Token *tok)
{
    for (LVar *var = locals; var != NULL; var = var->next)
    {
        if (var->len == tok->len && memcmp(tok->str, var->name, var->len) == 0)
        {
            return var;
        }
    }
    return NULL;
}

Node *new_node_ident(Token *ident)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar(ident);
    if (lvar != NULL)
    {
        // 存在済みの識別子
        node->offset = lvar->offset;
    }
    else
    {
        // 新しい識別子を割り当てる
        lvar = calloc(1, sizeof(LVar));
        lvar->next = locals; // 新しいものを先頭にする
        lvar->name = ident->str;
        lvar->len = ident->len;

        if (locals == NULL)
        {
            lvar->offset = 8; // 最初の値
        }
        else
        {
            lvar->offset = locals->offset + 8; // オフセットは増やしていく
        }
        node->offset = lvar->offset;
        locals = lvar;
    }

    return node;
}

Node *new_node_funccall(Token *ident)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FUNCCALL;
    node->fname = ident->str;
    node->fname_len = ident->len;
    return node;
}

void program();
Node *stmt();
Node *assign();
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

Node *code[100]; // stmt の配列

void program()
{
    // program = stmt*
    int i = 0;
    while (!at_eof())
    {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

// 文
Node *stmt()
{
    // stmt =
    //   expr ";" |
    //   "{" stmt* "}" |
    //   "if" "(" expr ")" stmt ("else" stmt)? |
    //   "while" "(" expr ")" stmt
    //   "for" "(" expr? ";" expr? ";" expr? ")" stmt |
    //   "return" expr ";" |

    if (consume_control(TK_IF))
    {
        Node *node = calloc(1, sizeof(Node));
        expect("("); // if の評価式には () を要求する
        // TODO: if の AST ノードはこれでいいのか？
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();

        if (consume_control(TK_ELSE))
        {
            // else がある場合
            node->kind = ND_IFELSE;
            node->opt_a = stmt();
        }
        else
        {
            // else がない場合
            node->kind = ND_IF;
        }
        return node;
    }

    if (consume_control(TK_WHILE))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("("); // while の評価式には () を要求する
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        return node;
    }

    if (consume_control(TK_FOR))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect("("); // for の評価式には () を要求する

        if (!consume_reserved(";"))
        {
            // 次のトークンが ';' ではなかった場合
            // for (A; ...) の A
            node->lhs = expr();
            expect(";"); // A の後には ; が来ていないといけない
        }
        else
        {
            node->lhs = NULL;
        }

        if (!consume_reserved(";"))
        {
            // 次のトークンが ';' ではなかった場合
            // for (A; B; ...) の B
            node->opt_a = expr();
            expect(";");
        }
        else
        {
            node->opt_a = NULL;
        }

        if (!consume_reserved(")"))
        {
            // 次のトークンが ')' ではなかった場合
            // for (A; B; C) の C
            node->opt_b = expr();
            expect(")");
        }
        else
        {
            node->opt_b = NULL;
        }

        node->rhs = stmt();
        return node;
    }

    if (consume_control(TK_RETURN))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr(); // TODO: ここで expr を期待しているので "return;" とは書けない
        expect(";");        // ; で終わっていなければエラー
        return node;
    }

    if (consume_reserved("{"))
    {
        Node *root = calloc(1, sizeof(Node));
        root->kind = ND_BLOCK;

        Node *prev = root;
        for (;;)
        {
            if (consume_reserved("}"))
            {
                // ブロック終了
                prev->next = NULL;
                break;
            }

            // root -> child1 -> child2 のように線形リストとしてぶら下げていく
            Node *child = stmt();
            prev->next = child;
            prev = child;
        }
        return root;
    }

    Node *node = expr();
    expect(";"); // ; で終わっていなければエラー
    return node;
}

// 式
Node *expr()
{
    // expr = assign
    return assign();
}

Node *assign()
{
    // assign = equality ("=" assign)?
    Node *node = equality();

    if (consume_reserved("="))
    {
        return new_node(ND_ASSIGN, node, assign());
    }

    return node;
}

Node *equality()
{
    // equality = relational ("==" relational | "!=" relational)*
    Node *node = relational();

    for (;;)
    {
        if (consume_reserved("=="))
            node = new_node(ND_EQ, node, relational());
        else if (consume_reserved("!="))
            node = new_node(ND_NE, node, relational());
        else
            return node;
    }
}

Node *relational()
{
    // relational = add ("<" add | "<=" add | ">" add | ">=" add)*
    Node *node = add();

    for (;;)
    {
        if (consume_reserved("<="))
            node = new_node(ND_LE, node, add());
        else if (consume_reserved("<"))
            node = new_node(ND_LT, node, add());
        else if (consume_reserved(">="))
            node = new_node(ND_GE, node, add());
        else if (consume_reserved(">"))
            node = new_node(ND_GT, node, add());
        else
            return node;
    }
}

Node *add()
{
    // add = mul ("+" mul | "-" mul)*
    Node *node = mul();

    for (;;)
    {
        if (consume_reserved("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume_reserved("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul()
{
    // mul = unary ("*" unary | "/" unary)*
    Node *node = unary();

    for (;;)
    {
        if (consume_reserved("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume_reserved("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary()
{
    // unary = (("+" | "-")? unary) | primary
    if (consume_reserved("+"))
    {
        return unary();
    }

    if (consume_reserved("-"))
    {
        // 0 - primary の AST とする
        return new_node(ND_SUB, new_node_num(0), unary());
    }

    return primary();
}

Node *primary()
{
    // primary =
    //   num |
    //   ident ("(" ")")? | // 引数なしの関数
    //  "(" expr ")"
    if (consume_reserved("("))
    {
        Node *node = expr();
        expect(")"); // () で囲まれていなければエラー
        return node;
    }

    Token *tok = consume_ident();
    if (tok != NULL)
    {
        if (consume_reserved("("))
        {
            expect(")");
            return new_node_funccall(tok);
        }
        return new_node_ident(tok);
    }

    return new_node_num(expect_number());
}

// パースの開始
Node **parse()
{
    program();
    return code;
}
