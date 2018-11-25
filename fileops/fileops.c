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

inline char *file_get_ext(char *path)
{
    return strrchr(path, '.');
}

/*return path to interpreter in *out buffer.
*return code:
*0: file is not interpretable
*1: file is interpretable and interpreter exists on system
*-1: error reading file
*-2: interpretable file but interpreter does not exist on system*/
int file_get_interpreter(char *path, char *out, size_t sz)
{
    //TODO: read from global config which interpreter to use first, then look in 1st line of script if none found
    char *ext = file_get_ext(path);

    char buf[1024] = "";
    FILE *fd = fopen(path, "r"); if (!fd) return -1;
    if (fread(buf, 1, sizeof(buf), fd) == 0) return -1;
    fclose(fd);

    if (buf[0] != '#' || buf[1] != '!') return 0; //file is not for interpreting
    for (int i = 0; buf[i+2] != '\n' && i < sz; ++i) {
        out[i] = buf[i+2];
    }
    if (file_executable(out) < 0) return -2; //no such interpreter on system
    
    return 1;
}

//check if file is regular or is directory.
int is_dir(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}


/*Get file size
 *Return size of file in bytes, or 0 on failure*/
size_t file_get_size(char *path)
{
    FILE *fd = fopen(path, "r");
    if (fd == NULL) return 0;
    fseek(fd, 0L, SEEK_END);
    size_t ret = ftell(fd);
    fclose(fd);
    return ret;
}