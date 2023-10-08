#pragma once

#include "parser.h"

// ノード列をアセンブルします。
void gen(Node *node);

// データセクションを生成します。
void gen_data();