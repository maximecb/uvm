size_t strlen(char* p)
{
    size_t l = 0;
    while (*(p + l) != 0) l = l + 1;
    return l;
}
