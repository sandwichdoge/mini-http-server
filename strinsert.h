#include <stdlib.h>
#include <string.h>

void str_insert(char *str, int pos, char *substr)
{
    int len = strlen(str);
    int sublen = strlen(substr);
    int taillen = len - pos;
    char *tail = malloc(taillen + 1);
    strncpy(tail, str + pos, taillen);
    memcpy(str + pos, substr, sublen);
    memcpy(str + pos + sublen, tail, taillen);
    *(str+pos+sublen+taillen) = 0;
    free(tail);
}