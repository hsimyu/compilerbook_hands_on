#include <stdio.h>
#include <stdlib.h>

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

int alloc4(int **outAddress, int a, int b, int c, int d)
{
    printf("alloc4\n");
    int *ptr = calloc(4, sizeof(int));
    ptr[0] = a;
    ptr[1] = b;
    ptr[2] = c;
    ptr[3] = d;
    *outAddress = ptr;
    return 0;
}