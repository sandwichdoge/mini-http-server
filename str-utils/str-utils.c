#include "str-utils.h"


/*insert substr into a position in str*/
void str_insert(char *str, int pos, char *substr)
{
    char *tail = malloc(strlen(str + pos) + 1);
    strcpy(tail, str + pos);
    str[pos] = '\0';
    strcat(str, substr);
    strcat(str, tail);
    free(tail);
}

/*find pos of character c in the first line of str*/
char *strchrln(char *str, char c)
{
    for (int i = 0; str[i] && str[i] != '\n'; ++i) {
        if (str[i] == c) {
            return &str[i];
        }
    }
    return NULL;
}
