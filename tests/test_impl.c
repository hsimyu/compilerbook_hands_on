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
    assert(1, line_comment());
    assert(42, block_comment());
}