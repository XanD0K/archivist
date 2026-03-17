#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
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
