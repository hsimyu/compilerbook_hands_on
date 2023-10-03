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

int main()
{
    assert(0, 0);
    assert(1, 0);
}