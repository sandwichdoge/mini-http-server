#include "fileops.h"


/*file operations, return 0 on success, -1 on error with errno*/
int file_exists(char *path)
{
    return access(path, F_OK);
}

int file_readable(char *path)
{
    return access(path, R_OK);
}

int file_writable(char *path)
{
    return access(path, W_OK);
}

int file_executable(char *path)
{
    return access(path, X_OK);
}


long file_get_size(char *path)
{
    FILE *fd = fopen(path, "r");
    fseek(fd, 0L, SEEK_END);
    long ret = ftell(fd);
    fclose(fd);
    return ret;
}