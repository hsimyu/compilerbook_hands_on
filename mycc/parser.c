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

// 現在定義中の関数ノード
Node *active_func;

// 組み込み型のリスト
Type type_list[100];

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

// 次のトークンが期待した型識別子の場合は、トークンを 1 つ読み進めてそのトークンを返す。
// それ以外の場合には NULL を返す。
Token *consume_type_ident(char *tname)
{
    if (token->kind != TK_IDENT ||
        strlen(tname) != token->len ||
        memcmp(token->str, tname, token->len))
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

// 次のトークンが期待している予約語かどうかを返す。
// トークンは進めない
bool peek_reserved(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;

    return true;
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

// 次のトークンが識別子の場合、トークンを 1 つ読み進めてそのトークンを返す。
// それ以外の場合には、エラーを報告する。
Token *expect_ident()
{
    if (token->kind != TK_IDENT)
        error_at(token->str, "Invalid TokenKind: excected = 'Identifier', actual = '%s'",
                 tokenKindToString(token->kind));

    Token *result = token;
    token = token->next;
    return result;
}

// 次のトークンが期待した識別子の場合、トークンを 1 つ読み進めてそのトークンを返す。
// それ以外の場合には、エラーを報告する。
Token *expect_type_ident(char *tname)
{
    if (token->kind != TK_IDENT ||
        strlen(tname) != token->len ||
        memcmp(token->str, tname, token->len))
        error_at(token->str, "Invalid TokenKind: excected = '%s', actual = '%.*s'", tname, token->len, token->str);

    Token *result = token;
    token = token->next;
    return result;
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

Type *find_type_impl(TypeKind kind, int ptr_depth)
{
    for (int i = 0; i < 100; i++)
    {
        Type *info = &type_list[i];
        if (info->kind == kind && info->ptr_depth == ptr_depth)
        {
            // 定義済みなので返す
            return &type_list[i];
        }
    }

    // 発見できなかった
    return NULL;
}

Type *new_type_impl(TypeKind kind, int ptr_depth)
{
    for (int i = 0; i < 100; i++)
    {
        if (type_list[i].kind == TYPE_INVALID)
        {
            type_list[i].kind = kind;
            type_list[i].ptr_depth = ptr_depth;
            return &type_list[i];
        }
    }

    error_at(token->str, "Max Type Count Exceed.");
    return NULL;
}

// 型名を名前引きする
// 見つからなかったら NULL を返します。
Type *search_type(char *tname, int ptr_depth)
{
    if (ptr_depth > 0)
    {
        Type *tdef = find_type_impl(TYPE_PTR, ptr_depth);
        if (tdef != NULL)
        {
            // 定義済みなので返す
            return tdef;
        }

        // 定義されていないので生成する
        // 深度を一つ減らして検索または生成する
        Type *ptr_to = search_type(tname, ptr_depth - 1);
        Type *newdef = new_type_impl(TYPE_PTR, ptr_depth);
        newdef->ptr_to = ptr_to;
        return tdef;
    }
    else
    {
        if (strncmp(tname, "int", 3) != 0)
        {
            // int 以外なら Error
            error_at(token->str, "Invalid type name.");
        }

        Type *int_def = find_type_impl(TYPE_INT, 0);
        if (int_def != NULL)
        {
            // int が定義済みなので返す
            return int_def;
        }
        else
        {
            // 型定義されている数を超えたので、定義して返す
            Type *newdef = new_type_impl(TYPE_INT, 0);
            return newdef;
        }
    }

    return NULL;
}

// 変数を名前引きする
// 見つからなかったら NULL を返します。
LVar *find_lvar(Token *tok)
{
    for (LVar *var = active_func->locals; var != NULL; var = var->next)
    {
        if (var->len == tok->len && memcmp(tok->str, var->name, var->len) == 0)
        {
            return var;
        }
    }
    return NULL;
}

Node *new_node_ident_ref(Token *ident)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR_REF;

    LVar *lvar = find_lvar(ident);
    if (lvar == NULL)
    {
        error_at(token->str - 1, "Undefined local variable is used: '%.*s'", ident->len, ident->str);
    }

    // 存在済みの識別子
    node->var_info = lvar;
    return node;
}

Node *new_node_ident_declare(Token *ident, int ptr_depth)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR_DEC;

    LVar *lvar = find_lvar(ident);
    if (lvar != NULL)
    {
        // 存在済みの識別子ならエラー
        error_at(token->str - 1, "Duplicate variable declaration: '%.*s'", ident->len, ident->str);
    }

    // 新しい識別子を割り当てる
    lvar = calloc(1, sizeof(LVar));
    lvar->next = active_func->locals; // 新しいものを先頭にする
    lvar->name = ident->str;
    lvar->len = ident->len;

    lvar->ty = search_type("int", ptr_depth);
    if (active_func->locals == NULL)
    {
        lvar->offset = 8; // 最初の値
    }
    else
    {
        lvar->offset = active_func->locals->offset + 8; // オフセットは増やしていく
    }
    node->var_info = lvar;
    active_func->locals = lvar;
    active_func->locals_count++;

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

Node *new_node_funcdef(Token *ident)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FUNCDEF;
    node->fname = ident->str;
    node->fname_len = ident->len;
    // TODO: すでに存在済みの関数かどうか調べる
    return node;
}

void program();
Node *funcdef();
Node *stmt();
Node *block();
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
    // プログラム == 関数定義の列ということにする
    // program = funcdef*
    int i = 0;
    while (!at_eof())
    {
        code[i++] = funcdef();
    }
    code[i] = NULL;
}

Node *funcdef()
{
    // funcdef = "int" ident "(" ("int" ident)? ("," "int" ident)? ")" block

    // 返り値の型: 今は読み捨てる
    expect_type_ident("int");

    Token *tok = expect_ident();
    Node *f = new_node_funcdef(tok);

    // active_func を切り替える
    Node *previous_func = active_func; // 復元用
    active_func = f;

    expect("(");

    // 関数の仮引数をパース
    // 仮引数は int x, int y のような形式で定義される
    tok = consume_type_ident("int");
    if (tok != NULL)
    {
        // NOTE: ここで仮引数列は LVAR ノードとして繋げられていくことに注意
        // レジスタの値を直接参照する場合は、LVAR 以外のノードを定義する必要がある
        tok = expect_ident();
        Node *arg = new_node_ident_declare(tok, 0);
        f->next = arg;
        f->arg_count++;

        // 第 2 引数以降
        while (consume_reserved(","))
        {
            // 型名は今は読み捨てる
            expect_type_ident("int");

            tok = expect_ident();
            arg->next = new_node_ident_declare(tok, 0);
            arg = arg->next;
            f->arg_count++;
        }

        arg->next = NULL;
    }

    // NOTE: ここで expect するのは型名または ) なので、expect(")") だとエラーメッセージが不適切
    expect(")");
    f->lhs = block();

    active_func = previous_func; // 元に戻す
    return f;
}

// 文
Node *stmt()
{
    // stmt =
    //   "int" "*"* ident ";" |
    //   "{" stmt* "}" |
    //   "if" "(" expr ")" stmt ("else" stmt)? |
    //   "while" "(" expr ")" stmt
    //   "for" "(" expr? ";" expr? ";" expr? ")" stmt |
    //   "return" expr ";" |
    //   expr ";"

    Token *type_token = consume_type_ident("int");
    if (type_token != NULL)
    {
        // ポインタ定義数をカウント
        int ptr_depth = 0;
        while (consume_reserved("*"))
        {
            ptr_depth++;
        }

        Token *ident = expect_ident();
        Node *node = new_node_ident_declare(ident, ptr_depth);
        expect(";");
        return node;
    }

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

    if (peek_reserved("{"))
    {
        return block();
    }

    Node *node = expr();
    expect(";"); // ; で終わっていなければエラー
    return node;
}

// 文ブロック
Node *block()
{
    // block = "{" stmt* "}"
    expect("{");

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
    // unary =
    //   "+"? unary |
    //   "-"? unary |
    //   "*" unary |
    //   "&" unary |
    //   primary
    if (consume_reserved("*"))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_DEREF;
        node->lhs = unary();
        return node;
    }

    if (consume_reserved("&"))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_ADDR;
        node->lhs = unary();
        return node;
    }

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
    //   ident ("(" expr? ("," expr)? ")")? |
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
            // 引数を評価
            Node *node = new_node_funccall(tok);

            if (consume_reserved(")"))
            {
                // 引数なしの呼び出し: f()
                return node;
            }

            // 第一引数があるはず
            node->next = expr();
            Node *prev = node->next;

            for (;;)
            {
                // 第一引数以降は , expr, expr, ...) で続く
                // ) が発見されるまで続ける
                if (consume_reserved(")"))
                {
                    prev->next = NULL;
                    break;
                }

                expect(",");
                prev->next = expr();
                prev = prev->next;
            }

            return node;
        }

        // 関数呼び出しでない
        return new_node_ident_ref(tok);
    }

    return new_node_num(expect_number());
}

// パースの開始
Node **parse()
{
    // 組み込み型リストを初期化
    for (int i = 0; i < 100; i++)
    {
        type_list[i].kind = TYPE_INVALID;
    }

    program();
    return code;
}
