int assert(int expected, int result)
{
    // printf() は標準ライブラリとして外部リンクされている
    char *msg;
    msg = "expected: %d, result: %d\n";
    printf(msg, expected, result);

    if (expected != result)
    {
        msg = "BAD\n";
        printf(msg);
        exit(1);
    }
    return 0;
}

int var_test_1()
{
    int a;
    a = 42;
    return a;
}

int var_test_2()
{
    int a;
    a = 1;
    int b;
    b = a + 20;
    return b + 43;
}

int if_test_1()
{
    int a;
    a = 1;
    if (a == 1)
        return 42;

    // 到達しない
    return -1;
}

int if_test_2()
{
    int a;
    a = -1;
    if (a == 1)
        return 42;

    return -1;
}

int if_test_3()
{
    int a;
    a = 0;

    if (a == 1)
        return 42;
    else
        return 35;

    // 到達しない
    return -1;
}

int while_test_1()
{
    int a;
    a = 1;
    while (a != 10)
        a = 10;
    return a;
}

int for_test_1()
{
    int a;
    a = 0;
    int b;
    for (b = 1; b <= 10; b = b + 1)
        a = a + b;
    return a;
}

int for_test_2()
{
    int a;
    a = 0;
    int b;
    for (b = 1; b <= 2; b = b + 1)
    {
        a = a + b;
        a = a * 2;
    }
    return a;
}

int foo() { return 42; }
int func_call_test1() { return foo() + 28; }

int foo2(int a, int b, int c) { return a + b + c; }
int func_call_test2() { return foo2(1, 2, 3); }

int ptr_test_1()
{
    int a;
    int *b;
    a = 42;
    b = &a;
    return *b;
}

int ptr_test_2()
{
    int a;
    int *b;
    int ********c;
    return 1;
}

int ptr_test_3()
{
    int x;
    int *y;
    y = &x;
    *y = 3;
    return x;
}

int ptr_test_4()
{
    int x;
    int *y;
    y = &x;
    int *z;
    z = &x;
    x = 3;
    return *y + *z;
}

// int a()
// {
//     int *p;
//     alloc4(&p, 1, 2, 4, 8);
//     int *q;
//     q = p + 2;
//     return *q;
// }

int sizeof_test_1()
{
    int x;
    return sizeof(x);
}

int sizeof_test_2()
{
    int *y;
    return sizeof(y);
}

int sizeof_test_3()
{
    int x;
    return sizeof(x + 3);
}

int sizeof_test_4()
{
    int *y;
    return sizeof(y + 3);
}

int sizeof_test_5()
{
    int x[10];
    return sizeof(x);
}

int array_test_1()
{
    int a[2];
    *a = 10;
    return *a;
}

int array_test_2()
{
    int a[2];
    *(a + 1) = 10;
    return *(a + 1);
}

int array_test_3()
{
    int a[2];
    *a = 16;
    *(a + 1) = 2;
    return *a;
}

int array_test_4()
{
    int a[2];
    *a = 1;
    *(a + 1) = 2;
    int *p;
    p = a;
    return *p + *(p + 1);
}

int array_test_5()
{
    int a[3];
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    return a[0] + a[1] + a[2];
}

/*
int global_var_test_a;
int global_var_test_1()
{
    global_var_test_a = 10;
    return global_var_test_a;
}

int global_var_test_b[10];
int global_var_test_2()
{
    global_var_test_b[0] = 1;
    global_var_test_b[1] = 2;
    return global_var_test_b[0] + global_var_test_b[1];
}
*/

int char_test_1()
{
    char a;
    a = 1;
    return a;
}

int char_test_2()
{
    char x[3];
    x[0] = -1;
    x[1] = 2;
    int y;
    y = 4;
    return x[0] + y;
}

int char_test_3()
{
    char x[3];
    x[0] = "aaa";
    return 1;
}

int line_comment()
{
    // 1 を返す
    return 1;
}

int block_comment()
{
    /* これはブロックコメントです */
    return 42;
}

int main()
{
    assert(0, 0);
    assert(21, 5 + 20 - 4);
    assert(47, 5 + 6 * 7);
    assert(15, 5 * (9 - 6));
    assert(4, (3 + 5) / 2);
    assert(10, -10 + 20);
    assert(10, --10);
    assert(10, --+10);
    assert(1, 5 == 5);
    assert(0, 5 != 5);
    assert(1, 4 != -1);
    assert(0, 1 > 5);
    assert(1, 10 > 1);
    assert(0, 5 < 5);
    assert(1, 1 <= 2);
    assert(0, 4 <= 2);
    assert(1, 4 <= 4);
    assert(1, 4 >= 4);
    assert(0, (20 + 3 - 40) >= -4);

    assert(42, var_test_1());
    assert(64, var_test_2());

    assert(42, if_test_1());
    assert(-1, if_test_2());
    assert(35, if_test_3());
    assert(10, while_test_1());
    assert(55, for_test_1());
    assert(8, for_test_2());

    assert(70, func_call_test1());
    assert(6, func_call_test2());

    assert(42, ptr_test_1());
    assert(1, ptr_test_2());
    assert(3, ptr_test_3());
    assert(6, ptr_test_4());

    assert(4, sizeof_test_1());
    assert(8, sizeof_test_2());
    assert(4, sizeof_test_3());
    assert(8, sizeof_test_4());
    assert(80, sizeof_test_5());

    assert(10, array_test_1());
    assert(10, array_test_2());
    assert(16, array_test_3());
    assert(3, array_test_4());
    assert(6, array_test_5());

    // assert(10, global_var_test_1());
    // assert(3, global_var_test_2());

    assert(1, char_test_1());
    assert(3, char_test_2());
    assert(1, char_test_3());

    assert(1, line_comment());
    assert(42, block_comment());
    return 0;
}