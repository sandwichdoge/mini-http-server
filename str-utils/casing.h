void lowercase(char *dst, char *src, int len)
{
    for (int i = 0; src[i] && i < len; ++i) {
        if (src[i] > 'A' && src[i] < 'Z') {
            dst[i] = src[i] - 'A' + 'a';
        }
        else {
            dst[i] = src[i];
        }
    }
}

void uppercase(char *dst, char *src, int len)
{
    for (int i = 0; src[i] && i < len; ++i) {
        if (src[i] > 'a' && src[i] < 'z') {
            dst[i] = src[i] - 'a' + 'A';
        }
        else {
            dst[i] = src[i];
        }
    }
}