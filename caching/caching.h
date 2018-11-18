#include <stdio.h>
#include <string.h>
#include "hashtable.h"
#include "../fileops/fileops.h"


/*Cache file into memory.
 *Return address of file content buffer, file size and filename
 *Everything is stored on the heap*/
cache_file_t* cache_add_file(char *path);