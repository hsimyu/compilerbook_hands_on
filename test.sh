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

assert_file() {
    expected="$1"
    filename="$2"

    ./bazel-bin/mycc/mycc $filename > asm/tmp.s
    mkdir -p outputs
    cc -static -o outputs/tmp asm/tmp.s
    ./outputs/tmp
    actual="$?"

    input=`cat $filename`

    if [ "$actual" = "$expected" ]; then
        echo "$input"
        echo "[Result] $actual"
    else
        echo "$input"
        echo "[Result] $expected expected, but got $actual"
        exit 1
    fi
    echo "-----------------------"
}

assert_str() {
    expected="$1"
    input="$2"

    test_file_name="outputs/autogen_test.c"
    echo "$input" > $test_file_name

    assert_file "$1" "$test_file_name"
}

assert_main() {
    assert_str "$1" "int main(){ $2 }"
}

# prepare
bazel build mycc

# test
# assert_str 4 'int main(){ int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q; }'
assert_str 3 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; return *a + *(a + 1); }'
# assert_str 3 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }'
# assert_str 6 'int main(){ int a[3]; a[0] = 1; a[1] = 2; a[2] = 3; return a[0] + a[1] + a[2]; }'
# assert_str 3 'int main(){ char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y; }'
run_test

echo OK