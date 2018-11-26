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


char* str_between(char *out, char *str, char l, char r)
{
    char *L = strchr(str, l);
    char *R = strchr(str, r);
    if (L == NULL || R == NULL) return NULL; //can't find one of 2 chars

    strncpy(out, L+1, R - L - 1);

    return R;
}


int chr_count(char *str, char c)
{
    int ret = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] == c) ret++;
    }
    return ret;
}