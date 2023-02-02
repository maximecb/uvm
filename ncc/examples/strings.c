size_t strlen(char* p)
{
    size_t l = 0;
    while (*(p + l) != 0) l = l + 1;
    return l;
}

bool streq(char* s1, char* s2)
{
    for (size_t i = 0;; i = i + 1)
    {
        char ch1 = *(s1 + i);
        char ch2 = *(s2 + i);

        if (ch1 != ch2)
            return 0;

        if (ch1 == 0)
            return 1;
    }
}

void main()
{
}
