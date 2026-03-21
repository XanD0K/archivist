#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "utils.h"

// Opens directory
DIR *open_directory(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr, "Couldn't open directory %s: %s\n", path, strerror(errno));
        return NULL;
    }
    return dir;
}

char *get_directory(char *path)
{
    if (!path)
    {
        return NULL;
    }
    char *base_dir = strdup(path);
    if (!base_dir)
    {
        return 6;
    }
    
    // Ensures path has no trailing /
    size_t len = strlen(base_dir);
    if (len > 0 && base_dir[len - 1] == '/')
    {
        base_dir[len - 1] = '\0';
    }

    return base_dir;
}