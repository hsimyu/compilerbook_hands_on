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

// 組み込み型のリスト
Type type_list[100];

// アドレスを返す性質があるノードかどうかを判定します。
bool is_address(Node *node)
{
    if (node == NULL)
    {
        return false;
    }

    if (node->kind == ND_ADDPTR || node->kind == ND_SUBPTR)
    {
        return true;
    }

    if (node->kind == ND_ADDR)
    {
        return true;
    }

    if (node->kind == ND_LVAR_REF && node->lvar_info->ty->kind == TYPE_PTR)
    {
        return true;
    }

    if (node->kind == ND_LVAR_REF && node->lvar_info->ty->kind == TYPE_ARRAY)
    {
        return true;
    }

    if (node->kind == ND_DEREF)
    {
        // TODO: lhs の ptr_to が PTR かどうかを調べるようにする
        return false;
    }

    return false;
}

bool is_array(Node *node)
{
    if (node == NULL)
    {
        return false;
    }

    return (node->kind == ND_LVAR_REF && node->lvar_info->ty->kind == TYPE_ARRAY);
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

Type *define_new_type_info(TypeKind kind, int ptr_depth)
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

int calc_type_size_impl(Type *ty)
{
    if (ty->kind == TYPE_ARRAY)
    {
        int elem_size = calc_type_size_impl(ty->ptr_to);
        return elem_size * ty->array_size;
    }

    if (ty->kind == TYPE_PTR)
    {
        return 8;
    }

    // INT
    return 8;
}

// 型名を名前引きする
// 見つからなかったら NULL を返します。
Type *search_type(char *tname, int ptr_depth, int array_size)
{
    if (array_size > 0)
    {
        Type *tdef = find_type_impl(TYPE_ARRAY, ptr_depth);
        if (tdef != NULL)
        {
            // 定義済み
            return tdef;
        }

        // 定義されていないので生成する
        // 配列型の時、同じポインタ深度の要素型の型情報を探し、それを指す Array 型情報を生成する
        // TODO: ポインタ深度を +1 すべきなのかどうか
        Type *ptr_to = search_type(tname, ptr_depth, 0);
        Type *newdef = define_new_type_info(TYPE_ARRAY, ptr_depth);
        newdef->ptr_to = ptr_to;
        newdef->array_size = array_size;
        newdef->type_size = calc_type_size_impl(newdef);
        return newdef;
    }

    if (ptr_depth > 0)
    {
        Type *tdef = find_type_impl(TYPE_PTR, ptr_depth);
        if (tdef != NULL)
        {
            // 定義済み
            return tdef;
        }

        // 定義されていないので生成する
        // 深度を一つ減らして検索または生成する
        Type *ptr_to = search_type(tname, ptr_depth - 1, 0);
        Type *newdef = define_new_type_info(TYPE_PTR, ptr_depth);
        newdef->ptr_to = ptr_to;
        newdef->type_size = calc_type_size_impl(newdef);
        return newdef;
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
            Type *newdef = define_new_type_info(TYPE_INT, 0);
            newdef->type_size = calc_type_size_impl(newdef);
            return newdef;
        }
    }

    return NULL;
}

void init_type()
{
    // 組み込み型リストを初期化
    for (int i = 0; i < 100; i++)
    {
        type_list[i].kind = TYPE_INVALID;
    }
}
