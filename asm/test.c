#include <stdio.h>

int foo()
{
    printf("OK\n");
    return 0;
}

int arg1(int a)
{
    printf("a = %d\n", a);
    return a;
}

int arg2(int a, int b)
{
    printf("a = %d, b = %d\n", a, b);
    return a + b;
}

int arg3(int a, int b, int c)
{
    printf("a = %d, b = %d, c = %d\n", a, b, c);
    return a + b + c;
}