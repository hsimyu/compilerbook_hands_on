#pragma once

// エラー箇所を報告します。
void error_at(char *loc, char *fmt, ...);

// エラーを出力します。
void error(char *fmt, ...);

// 指定されたパスにあるファイルの内容を読み込み、返します。
char *read_file(char *path);