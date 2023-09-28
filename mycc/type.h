#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef enum
{
    TYPE_INT,
    TYPE_CHAR,
    TYPE_PTR,
    TYPE_ARRAY,
    TYPE_INVALID,
} TypeKind;

typedef struct Type Type;
struct Type
{
    TypeKind kind;     // 型の種類
    int type_size;     // 型のサイズ
    Type *ptr_to;      // PTR: ポインタ型の指し先の型
    int ptr_depth;     // PTR: ポインタの深度 (* なら 1, ** なら 2), ptr_to をたどっていくようにすれば定義不要
    size_t array_size; // ARRAY: 固定長配列サイズ
};

struct Node;
bool is_address(struct Node *node);
bool is_array(struct Node *node);

void init_type();
Type *search_type(char *tname, int ptr_depth, int array_size);