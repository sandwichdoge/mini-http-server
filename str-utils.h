#include <stdlib.h>
#include <string.h>

void str_insert(char *str, int pos, char *substr)
{
    char *tail = malloc(strlen(str + pos));
    strcpy(tail, str + pos);
    str[pos] = '\0';
    strcat(str, substr);
    strcat(str, tail);
    free(tail);
}