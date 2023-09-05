#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 入力プログラム
char *user_input;

// エラー箇所を報告します。
void error_at(char *loc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " "); // pos 個の空白
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラーを出力します。
void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

typedef enum
{
    TK_RESERVED, // 記号
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わり
} TokenKind;

typedef struct Token Token;

const char *tokenKindToString(TokenKind kind)
{
    switch (kind)
    {
    case TK_RESERVED:
        return "Reserved";
    case TK_NUM:
        return "Number";
    case TK_EOF:
        return "EOF";
    }

    return "Unknown";
}

struct Token
{
    TokenKind kind; // トークン型
    Token *next;    // 次の入力トークン
    int val;        // TK_NUM の場合、その数値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
};

// 現在着目しているトークン
Token *token;

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて true を返す。
// それ以外の場合には false を返す
bool consume(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
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

// 次のトークンの文字列長を調べます。
int count_token(char *str)
{
    if (strncmp(str, "==", 2) == 1 ||
        strncmp(str, "!=", 2) == 1 ||
        strncmp(str, "<=", 2) == 1 ||
        strncmp(str, ">=", 2) == 1)
    {
        return 2;
    }
    else
    {
        return 1;
    }
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

// 入力文字列 p をトークナイズして、それを返します。
Token *tokenize(char *p)
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

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '=' || *p == '!' || *p == '<' || *p == '>')
        {
            int token_length = count_token(p);
            cur = new_token(TK_RESERVED, cur, p, token_length);
            p = p + token_length;
            continue;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p, 1); // ここ 1 でいいの?
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(cur->str + 1, "Failed to tokenize: %c", *p);
    }

    new_token(TK_EOF, cur, p, 1);
    return head.next;
}

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

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

Node *expr()
{
    // expr = equality
    return equality();
}

Node *equality()
{
    // equality = relational ("==" relational | "!=" relational)*
    Node *node = relational();

    for (;;)
    {
        if (consume("=="))
            node = new_node(ND_EQ, node, relational());
        else if (consume("!="))
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
        if (consume("<="))
            node = new_node(ND_LE, node, add());
        else if (consume("<"))
            node = new_node(ND_LT, node, add());
        else if (consume(">="))
            node = new_node(ND_GE, node, add());
        else if (consume(">"))
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
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-"))
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
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary()
{
    // unary = ("+" | "-")? primary
    if (consume("+"))
    {
        return primary();
    }

    if (consume("-"))
    {
        // 0 - primary の AST とする
        return new_node(ND_SUB, new_node_num(0), primary());
    }

    return primary();
}

Node *primary()
{
    // primary = num | "(" expr ")"
    if (consume("("))
    {
        Node *node = expr();
        expect(")"); // () で囲まれていなければエラー
        return node;
    }

    return new_node_num(expect_number());
}

// assembler
void gen(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind)
    {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        // imul は rdx と rax を取って 128 ビットの値とし、
        // 引数のレジスタで割り、
        // 商を rax に、余りを rdx にセットする命令
        // cqo: rax に入っている 64bit の値を 128bit に伸ばして rdx と rax にセットする
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    default:
        error("Failed to generate assembly: unknown node = %d", node->kind);
        break;
    }

    printf("  push rax\n");
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Invalid argument count: arg must be 1.\n");
        return 1;
    }

    // トークン列を生成
    user_input = argv[1];
    token = tokenize(argv[1]);
    Node *node = expr();

    // アセンブリのヘッダー
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // AST -> ASM
    gen(node);

    // 関数からの返り値としてスタックトップの値を rax にロードして返す
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}