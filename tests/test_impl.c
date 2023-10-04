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

    assert(1, line_comment());
    assert(42, block_comment());
}