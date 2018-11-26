#ifndef _str_utils_h_
#define _str_utils_h_
#include <stdlib.h>
#include <string.h>

/*insert substr into a position in str*/
void str_insert(char *str, int pos, char *substr);

/*find pos of character c in the first line of str*/
char *strchrln(char *str, char c);

/*strip trailing CRLF*/
int strip_trailing_lf(char *str, int how_many);

/*copy str between l and r to out buffer*/
char *str_between(char *out, char *str, char l, char r);

/*count chars in str*/
int chr_count(char *str, char c);

#endif