#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

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
