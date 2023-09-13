#include <stdio.h>

int foo()
{
    printf("OK\n");
    return 0;
}

int args1(int a)
{
    printf("a = %d\n", a);
    return a;
}

int args2(int a, int b)
{
    printf("a = %d, b = %d\n", a, b);
    return a + b;
}

int args3(int a, int b, int c)
{
    printf("a = %d, b = %d, c = %d\n", a, b, c);
    return a + b + c;
}