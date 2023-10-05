#!/bin/bash
run_test() {
    # テストを自作コンパイラでビルドし、走らせる
    ./bazel-bin/mycc/mycc "tests/test_impl.c" > asm/tmp.s
    mkdir -p outputs
    cc -static -o outputs/tmp asm/tmp.s
    ./outputs/tmp

    exitcode="$?"
    if [ $exitcode -eq 1 ]; then
        echo "[Result] Test Failed."
        exit 1
    fi
}

# prepare
bazel build mycc

# test
# assert_str 4 'int main(){ int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q; }'
run_test

echo OK