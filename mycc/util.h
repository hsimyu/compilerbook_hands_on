#pragma once

// エラー箇所を報告します。
void error_at(char *loc, char *fmt, ...);

// エラーを出力します。
void error(char *fmt, ...);