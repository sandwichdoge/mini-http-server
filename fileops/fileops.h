#ifndef _fileops_h
#define _fileops_h

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

size_t file_get_size(char *path);
int file_exists(char *path);
int file_readable(char *path);
int file_writable(char *path);
int file_executable(char *path);
char *file_get_ext(char *path);
int is_dir(char *path);

#endif