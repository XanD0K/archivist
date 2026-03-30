#ifndef UTILS_H
#define UTILS_H

// Libraries
#include <dirent.h>
#include <stddef.h>
#include <sys/types.h>

// Prototypes
char *get_valid_directory(const char *path);
char *formatted_output(off_t total_size);
const char *get_extension(const char *name);
bool check_help(int argc, char *argv);
bool check_sort(char *sort, const char **sorts, size_t len);
off_t get_size(const char *size);

#endif
