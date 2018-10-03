#ifndef _fileops_h
#define _fileops_h

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

long file_get_size(char *path);
int file_exists(char *path);
int file_readable(char *path);
int file_writable(char *path);
int file_executable(char *path);
int file_get_interpreter(char *path, char *out, size_t sz);
int is_dir(char *path);

#endif