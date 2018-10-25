#ifndef _str_utils_h_
#define _str_utils_h_
#include <stdlib.h>
#include <string.h>

/*insert substr into a position in str*/
void str_insert(char *str, int pos, char *substr);

/*find pos of character c in the first line of str*/
char *strchrln(char *str, char c);

#endif