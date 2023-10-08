#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "tokenizer.h"
#include "parser.h"
#include "type.h"

extern Token *token;

// 現在定義中の関数ノード
Node *active_func;

// グローバル変数のリスト
GVar *globals;
GVar *get_gvar()
{
    return globals;
}

// 文字列リテラルのリスト
StringLiteral *str_literals;
StringLiteral *get_string_literals()
{
    return str_literals;
}

// 変数宣言情報
typedef struct DeclToken DeclToken;
struct DeclToken
{
    Token *type_token;
    Token *identifier_token;
    int ptr_depth;  // ポインタの場合
    int array_size; // array の場合
};

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

// 次のトークンが型識別子の場合は、トークンを 1 つ読み進めてそのトークンを返す。
// それ以外の場合には NULL を返す。
// NOTE: これを別定義にしておかないと、int a; と a=0; のようなものを区別できない……
Token *consume_type_ident()
{
    if (token->kind != TK_IDENT)
        return NULL;

    if (token->len == 3 && memcmp(token->str, "int", token->len) == 0)
    {
        Token *result = token;
        token = token->next;
        return result;
    }

    if (token->len == 4 && memcmp(token->str, "char", token->len) == 0)
    {
        Token *result = token;
        token = token->next;
        return result;
    }

    return NULL;
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

// 次のトークンが文字列の場合は、トークンを 1 つ読み進めてそのトークンを返す。
// それ以外の場合には NULL を返す。
Token *consume_string()
{
    if (token->kind != TK_STRING)
        return NULL;

    Token *result = token;
    token = token->next;
    return result;
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

Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs)
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
    node->val_num = val;
    return node;
}

// グローバル変数を名前引きする
// 見つからなかったら NULL を返します。
GVar *find_gvar(Token *tok)
{
    for (GVar *var = globals; var != NULL; var = var->next)
    {
        if (var->len == tok->len && memcmp(tok->str, var->name, var->len) == 0)
        {
            return var;
        }
    }
    return NULL;
}

// 現在アクティブな関数スコープからローカル変数を名前引きする
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

Node *new_node_var_ref(Token *ident)
{
    Node *node = calloc(1, sizeof(Node));
    LVar *lvar = find_lvar(ident);
    if (lvar == NULL)
    {
        // ローカル変数が見つからなければ、グローバル変数を探す
        GVar *gvar = find_gvar(ident);

        if (gvar == NULL)
            error_at(token->str - 1, "Undefined variable is used: '%.*s'", ident->len, ident->str);

        // グローバル変数だった
        node->kind = ND_GVAR_REF;
        node->gvar_info = gvar;
    }
    else
    {
        // ローカル変数だった
        node->kind = ND_LVAR_REF;
        node->lvar_info = lvar;
    }
    return node;
}

Node *new_node_lvar_declare(DeclToken *decl_token)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR_DEC;

    Token *ident = decl_token->identifier_token;
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

    lvar->ty = search_type(decl_token->type_token->str, decl_token->type_token->len, decl_token->ptr_depth, decl_token->array_size);
    if (lvar->ty == NULL)
    {
        error_at(token->str - 1, "Compiler error: failed to declare = '%.*s'", ident->len, ident->str);
    }

    int size_on_stack = lvar->ty->type_size;
    if (active_func->locals == NULL)
    {
        lvar->offset = size_on_stack; // 最初の変数は rbp - size の位置になる
    }
    else
    {
        // 新しいオフセットは、ここまでの変数サイズ + size
        lvar->offset = active_func->locals_size + size_on_stack;
    }
    node->lvar_info = lvar;
    active_func->locals = lvar;

    active_func->locals_size += size_on_stack;

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
    // TODO: 返り値型に対応
    return node;
}

Node *new_node_gvar(DeclToken *decl_token)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_GVAR_DEF;

    Token *ident = decl_token->identifier_token;
    GVar *gvar = find_gvar(ident);
    if (gvar != NULL)
    {
        // 存在済みの識別子ならエラー
        error_at(token->str - 1, "Duplicate global variable declaration: '%.*s'", ident->len, ident->str);
    }

    // 新しい識別子を割り当てる
    gvar = calloc(1, sizeof(LVar));
    gvar->next = globals; // 新しいものを先頭にする
    gvar->name = ident->str;
    gvar->len = ident->len;

    gvar->ty = search_type(decl_token->type_token->str, decl_token->type_token->len, decl_token->ptr_depth, decl_token->array_size);
    if (gvar->ty == NULL)
    {
        error_at(token->str - 1, "Compiler error: failed to declare = '%.*s'", ident->len, ident->str);
    }

    node->gvar_info = gvar;
    globals = gvar;

    return node;
}

void program();
DeclToken *decl();
Node *symboldef();
Node *stmt();
Node *block();
Node *assign();
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *array_index();
Node *primary();

Node *code[100]; // stmt の配列

void program()
{
    // プログラム == シンボル定義の列ということにする
    // program = symboldef*
    int i = 0;
    while (!at_eof())
    {
        code[i++] = symboldef();
    }
    code[i] = NULL;
}

DeclToken *decl()
{
    // decl = ("int" | "char") "*"* ident ("[" num "]")?
    Token *type_token = consume_type_ident();
    if (type_token != NULL)
    {
        DeclToken *new_decl = calloc(1, sizeof(DeclToken));
        new_decl->type_token = type_token;

        // ポインタ定義数をカウント
        int ptr_depth = 0;
        while (consume_reserved("*"))
        {
            ptr_depth++;
        }
        new_decl->ptr_depth = ptr_depth;

        Token *ident = expect_ident();
        new_decl->identifier_token = ident;

        if (consume_reserved("["))
        {
            int array_size = expect_number();
            // TODO: array_size が正の整数でなければ怒る

            new_decl->array_size = array_size;
            // 配列定義
            expect("]");
        }

        return new_decl;
    }
    return NULL;
}

Node *symboldef()
{
    // symboldef =
    // decl ";" | グローバル変数定義
    // decl "(" (decl)? ("," decl)? ")" block | 関数定義

    // 関数の返り値の型か、グローバル変数の型
    DeclToken *new_decl = decl();
    if (peek_reserved(";"))
    {
        // グローバルな変数定義である
        Node *node = new_node_gvar(new_decl);
        expect(";");
        return node;
    }

    // 関数定義である
    Node *f = new_node_funcdef(new_decl->identifier_token);

    // active_func を切り替える
    Node *previous_func = active_func; // 復元用
    active_func = f;
    active_func->locals = NULL;
    active_func->locals_size = 0;

    expect("(");

    // 関数の仮引数をパース
    // 仮引数は int x, int* y のような形式で定義される
    DeclToken *arg_decl = decl();
    if (arg_decl != NULL)
    {
        // NOTE: ここで仮引数列は LVAR ノードとして繋げられていくことに注意
        // レジスタの値を直接参照する場合は、LVAR 以外のノードを定義する必要がある
        Node *arg = new_node_lvar_declare(arg_decl);
        f->next = arg;
        f->arg_count++;

        // 第 2 引数以降
        while (consume_reserved(","))
        {
            // 型名は今は読み捨てる
            arg_decl = decl();
            if (arg_decl == NULL)
                error_at(token->str, "Invalid Function Argument.");

            arg->next = new_node_lvar_declare(arg_decl);
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
    //   decl ";" |
    //   "{" stmt* "}" |
    //   "if" "(" expr ")" stmt ("else" stmt)? |
    //   "while" "(" expr ")" stmt
    //   "for" "(" expr? ";" expr? ";" expr? ")" stmt |
    //   "return" expr ";" |
    //   expr ";"

    DeclToken *new_decl = decl();
    if (new_decl != NULL)
    {
        Node *node = new_node_lvar_declare(new_decl);
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
        return new_node_binary(ND_ASSIGN, node, assign());
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
            node = new_node_binary(ND_EQ, node, relational());
        else if (consume_reserved("!="))
            node = new_node_binary(ND_NE, node, relational());
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
            node = new_node_binary(ND_LE, node, add());
        else if (consume_reserved("<"))
            node = new_node_binary(ND_LT, node, add());
        else if (consume_reserved(">="))
            node = new_node_binary(ND_GE, node, add());
        else if (consume_reserved(">"))
            node = new_node_binary(ND_GT, node, add());
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
        {
            if (is_address(node))
            {
                node = new_node_binary(ND_ADDPTR, node, mul());
            }
            else
            {
                node = new_node_binary(ND_ADD, node, mul());
            }
        }
        else if (consume_reserved("-"))
        {
            if (is_address(node))
            {
                node = new_node_binary(ND_SUBPTR, node, mul());
            }
            else
            {
                node = new_node_binary(ND_SUB, node, mul());
            }
        }
        else
        {
            return node;
        }
    }
}

Node *mul()
{
    // mul = unary ("*" unary | "/" unary)*
    Node *node = unary();

    for (;;)
    {
        if (consume_reserved("*"))
            node = new_node_binary(ND_MUL, node, unary());
        else if (consume_reserved("/"))
            node = new_node_binary(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary()
{
    // unary =
    //   "sizeof" unary |
    //   "+"? unary |
    //   "-"? unary |
    //   "*" unary |
    //   "&" unary |
    //   primary
    if (consume_control(TK_SIZEOF))
    {
        Node *rhs = unary();
        int type_size = 0;
        if (is_address(rhs))
        {
            if (is_array(rhs))
            {
                // array
                type_size = rhs->lvar_info->ty->type_size;
            }
            else
            {
                // PTR
                type_size = 8;
            }
        }
        else
        {
            // INT
            type_size = 4;
        }

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_NUM;
        node->val_num = type_size;
        return node;
    }

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
        // 0 - unary の AST とする
        return new_node_binary(ND_SUB, new_node_num(0), unary());
    }

    return array_index();
}

Node *array_index()
{
    // array_index = primary ( "[" primary "]" )?
    Node *p1 = primary();

    if (consume_reserved("["))
    {
        // p1[p2] は *(p1 + p2) と読み替える
        Node *p2 = primary();
        expect("]");

        Node *addptr = calloc(1, sizeof(Node));
        addptr->kind = ND_ADDPTR;
        addptr->lhs = p1;
        addptr->rhs = p2;

        Node *deref = calloc(1, sizeof(Node));
        deref->kind = ND_DEREF;
        deref->lhs = addptr;
        return deref;
    }
    else
    {
        return p1;
    }
}

Node *primary()
{
    // primary =
    //   num |
    //   str |
    //   ident ("(" expr? ("," expr)* ")")? |
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
                node->call_args = NULL;
                // 引数なしの呼び出し: f()
                return node;
            }

            // 第一引数があるはず
            node->call_args = expr();
            Node *target = node->call_args;

            int c = 1;
            for (;;)
            {
                // 第一引数以降は , expr, expr, ...) で続く
                // ) が発見されるまで続ける
                if (consume_reserved(")"))
                {
                    target->call_args = NULL;
                    break;
                }

                expect(",");
                target->call_args = expr();
                target = target->call_args;
                c++;
            }

            return node;
        }

        // 関数呼び出しでない
        return new_node_var_ref(tok);
    }

    tok = consume_string();
    if (tok)
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_STRING;

        // 生成済みの文字列リテラルを探索
        // NOTE: ここで、Node は "abc" のように二重引用符まで含めた文字列を保持している
        int content_length = (tok->len - 2);
        StringLiteral *target = str_literals;
        while (target != NULL)
        {
            if (target->len == content_length && memcmp(tok->str + 1, target->val, content_length))
                break;

            target = target->next;
        }

        if (target == NULL)
        {
            // 新しい文字列
            target = calloc(1, sizeof(StringLiteral));
            target->val = tok->str + 1;
            target->len = content_length;
            node->val_str = target;

            // この実装だと、先に出てきたリテラルの方が後ろに定義されることに注意
            target->next = str_literals;
            str_literals = target;
        }
        else
        {
            // 既知の文字列
            node->val_str = target;
        }

        return node;
    }

    return new_node_num(expect_number());
}

// パースの開始
Node **parse()
{
    init_type();
    program();
    return code;
}
