#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Headers
#include "utils.h"

// Opens directory
DIR *open_directory(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        fprintf(stderr, "Couldn't open directory %s: %s\n", path, strerror(errno));
        return NULL;
    }
    return dir;
}

// Checks for valid directory, setting (.) as default, and removing trailing /
char *get_valid_directory(const char *path)
{
    const char *p = (!path) ? "." : path;
    char *base_dir = strdup(p);
    if (!base_dir)
    {
        return NULL;
    }

    struct stat st;
    // Tries to fill st with directory's data
    if (stat(base_dir, &st) != 0)
    {
        fprintf(stderr, "Error in stat() for %s: %s\n", p, strerror(errno));
        free(base_dir);
        return NULL;
    }

    // Checks if path is a directory
    if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        fprintf(stderr, "Error accessing diretory %s: %s\n", p, strerror(errno));
        free(base_dir);
        return NULL;
    }

    // Ensures path has no trailing /
    size_t len = strlen(base_dir);
    if (len > 0 && base_dir[len - 1] == '/')
    {
        base_dir[len - 1] = '\0';
    }

    return base_dir;
}

// Prints a more readable output message
void formatted_output(size_t total_size)
{
    static const char *sizes[] = {"bytes", "kilobytes", "megabytes", "gigabytes", "terabytes", "petabytes"};
    const float buffer = 1024.0;

    float result = (float)total_size;
    size_t index = 0;    
    
    while (result >= buffer)
    {
        result /= buffer;
        index++;
    }

    // Limits index to 5
    index = (index < 5) ? index : 5;

    printf("Total size: %.2f %s\n", result, sizes[index]);
}

// Gets extension
char *get_extension(char *name)
{
    const char *dot = strrchr(name, '.');
    const char *ext = (dot != NULL) ? dot + 1 : "";
    return ext;
}
