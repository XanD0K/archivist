#ifndef UTILS_H
#define UTILS_H

// Libraries
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>

// Prototypes
char **validade_command(char *command);
bool validate_args(int argc, char *command, ptrdiff_t index);
DIR *open_directory(const char *path);



// Globals
extern char *commands[];

#endif
