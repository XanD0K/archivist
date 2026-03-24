#ifndef UTILS_H
#define UTILS_H

// Libraries
#include <dirent.h>

// Prototypes
DIR *open_directory(const char *path);
char *get_valid_directory(const char *path);

#endif
