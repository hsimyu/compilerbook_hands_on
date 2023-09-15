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

# prepare
cd asm
cc -c test test.c
cd ..
bazel build mycc

# test
assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 10 "-10+20;"
assert 10 '- -10;'
assert 10 '- - +10;'
assert 1 '5 == 5;'
assert 0 '5 != 5;'
assert 1 '4 != -1;'
assert 0 '5 < 5;'
assert 1 '1 < 2;'
assert 1 '1 <= 2;'
assert 0 '4 <= 2;'
assert 1 '4 > 2;'
assert 1 '4 >= 4;'
assert 0 '(20 + 3 - 40) >= -4;'
assert 2 '1;2;'
assert 42 'a=42;'
assert 42 'a=42;a;'
assert 32 'a=42;a=32;return a;'
assert 21 'a=1;b=a+20;'
assert 64 'a=1;b=a+20;return b+43;'
assert 42 'a=1;if (a==1) return 42;'
assert 35 'a=0;if (a==1) return 42; else return 35;'
assert 11 'a=1;while(a<=10)a=11;return a;'
assert 55 'a=0;for(b=1;b<=10;b=b+1)a=a+b;return a;'
assert 8 'a=0;for(b=1;b<=2;b=b+1){a=a+b;a=a*2;} return a;'
assert 0 'foo();'
assert 1 'return arg1(1);'
assert 5 'return arg2(1, 4);'
assert 6 'return arg3(1, 2, 3);'

echo OK