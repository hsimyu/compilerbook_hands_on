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
assert_str 70 'int foo() {return 42;} int main(){ return foo() + 28; }'
assert_str 6 'int foo(int a, int b, int c) {return a + b + c;} int main(){ return foo(1, 2, 3); }'
assert_str 42 'int main(){ int a; int b; a=42; b=&a; return *b; }'
assert_str 1 'int main(){ int a; int* b; int******** c; return 1; }'
assert_str 3 'int main(){ int x; int *y; y = &x; *y = 3; return x; }'
assert_str 6 'int main(){ int x; int *y; y = &x; int *z; z = &x; x = 3; return *y + *z; }'
# assert_str 4 'int main(){ int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q; }'
assert_str 4 'int main(){ int x; return sizeof(x); }'
assert_str 8 'int main(){ int *y; return sizeof(y); }'
assert_str 4 'int main(){ int x; return sizeof(x + 3); }'
assert_str 8 'int main(){ int *y; return sizeof(y + 3); }'
assert_str 80 'int main(){ int x[10]; return sizeof(x); }'
assert_str 10 'int main(){ int a[2]; *a = 10; return *a; }'
assert_str 10 'int main(){ int a[2]; *(a + 1) = 10; return *(a + 1); }'
assert_str 3 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; return *a + *(a + 1); }'
assert_str 3 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }'
assert_str 6 'int main(){ int a[3]; a[0] = 1; a[1] = 2; a[2] = 3; return a[0] + a[1] + a[2]; }'
assert_str 1 'int a; int b; int z[10]; int main(){ return 1; }'
assert_str 0 'int a; int main(){ return a; }'
assert_str 1 'int a; int main(){ a = 1; return a; }'
assert_str 1 'int main(){ char a; a = 1; return a; }'
assert_str 3 'int main(){ char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y; }'
assert_str 1 'int main(){ char x[3]; x[0] = "aaa"; return 1; }'
run_test

echo OK