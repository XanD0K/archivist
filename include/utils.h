#ifndef UTILS_H
#define UTILS_H

// Libraries
#include <dirent.h>
#include <stddef.h>

// Prototypes
DIR *open_directory(const char *path);
char *get_valid_directory(const char *path);
void formated_output(size_t total_size);
char *get_extension(char *name);

#endif
