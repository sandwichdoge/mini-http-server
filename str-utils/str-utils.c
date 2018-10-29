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

/*strip trailing CRLF*/
int strip_trailing_lf(char *str, int how_many)
{
    int len = 0;
    for (int i = 0; i < how_many; i++) {
        len = strlen(str);
        if (strcmp(&str[len-2], "\r\n") == 0) {
            str[len-2] = '\0';
        }
        else if (strcmp(&str[len-1], "\n") == 0 ) {
            str[len-1] = '\0';
        }
    }
    
    return 0;
}