#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./bazel-bin/mycc/mycc "$input" > asm/tmp.s
    mkdir -p outputs
    cc -o outputs/tmp asm/tmp.s asm/test.o
    ./outputs/tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert_main() {
    assert "$1" "int main(){ $2 }"
}

# prepare
cd asm
cc -c test test.c
cd ..
bazel build mycc

# test
assert_main 0 "0;"
assert_main 42 "42;"
assert_main 21 "5+20-4;"
assert_main 41 " 12 + 34 - 5 ;"
assert_main 47 "5+6*7;"
assert_main 15 "5*(9-6);"
assert_main 4 "(3+5)/2;"
assert_main 10 "-10+20;"
assert_main 10 '- -10;'
assert_main 10 '- - +10;'
assert_main 1 '5 == 5;'
assert_main 0 '5 != 5;'
assert_main 1 '4 != -1;'
assert_main 0 '5 < 5;'
assert_main 1 '1 < 2;'
assert_main 1 '1 <= 2;'
assert_main 0 '4 <= 2;'
assert_main 1 '4 > 2;'
assert_main 1 '4 >= 4;'
assert_main 0 '(20 + 3 - 40) >= -4;'
assert_main 2 '1;2;'
assert_main 42 'int a; a=42;'
assert_main 42 'int a; a=42;a;'
assert_main 32 'int a; a=42;a=32;return a;'
assert_main 21 'int a; a=1; int b; b=a+20;'
assert_main 64 'int a; a=1; int b; b=a+20;return b+43;'
assert_main 42 'int a; a=1;if (a==1) return 42;'
assert_main 35 'int a; a=0;if (a==1) return 42; else return 35;'
assert_main 10 'int a; a=1;while(a!=10)a=10;return a;'
assert_main 55 'int a; a=0;int b;for(b=1;b<=10;b=b+1)a=a+b;return a;'
assert_main 8 'int a; a=0;int b;for(b=1;b<=2;b=b+1){a=a+b;a=a*2;} return a;'
assert_main 0 'foo();'
assert_main 1 'return arg1(1);'
assert_main 5 'return arg2(1, 4);'
assert_main 6 'return arg3(1, 2, 3);'
assert 70 'int foo() {return 42;} int main(){ return foo() + 28; }'
assert 6 'int foo(int a, int b, int c) {return a + b + c;} int main(){ return foo(1, 2, 3); }'
assert 42 'int main(){ int a; int b; a=42; b=&a; return *b; }'
assert 1 'int main(){ int a; int* b; int******** c; return 1; }'
assert 3 'int main(){ int x; int *y; y = &x; *y = 3; return x; }'
assert 6 'int main(){ int x; int *y; y = &x; int *z; z = &x; x = 3; return *y + *z; }'
assert 4 'int main(){ int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q; }'
assert 4 'int main(){ int x; return sizeof(x); }'
assert 8 'int main(){ int *y; return sizeof(y); }'
assert 4 'int main(){ int x; return sizeof(x + 3); }'
assert 8 'int main(){ int *y; return sizeof(y + 3); }'
assert 80 'int main(){ int x[10]; return sizeof(x); }'
assert 10 'int main(){ int a[2]; *a = 10; return *a; }'
assert 10 'int main(){ int a[2]; *(a + 1) = 10; return *(a + 1); }'
assert 3 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; return *a + *(a + 1); }'
assert 3 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }'

echo OK