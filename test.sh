#!/bin/bash
assert_main() {
    expected="$1"
    input="main(){ $2 }"

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
assert_main 42 'a=42;'
assert_main 42 'a=42;a;'
assert_main 32 'a=42;a=32;return a;'
assert_main 21 'a=1;b=a+20;'
assert_main 64 'a=1;b=a+20;return b+43;'
assert_main 42 'a=1;if (a==1) return 42;'
assert_main 35 'a=0;if (a==1) return 42; else return 35;'
assert_main 11 'a=1;while(a<=10)a=11;return a;'
assert_main 55 'a=0;for(b=1;b<=10;b=b+1)a=a+b;return a;'
assert_main 8 'a=0;for(b=1;b<=2;b=b+1){a=a+b;a=a*2;} return a;'
assert_main 0 'foo();'
assert_main 1 'return arg1(1);'
assert_main 5 'return arg2(1, 4);'
assert_main 6 'return arg3(1, 2, 3);'

echo OK