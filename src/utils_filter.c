// Libraries
#include <strings.h>
#include <sys/stat.h>

// Headers
#include "utils_filter.h"

// Checks if type matches
bool match_type(const char *type, mode_t mode)
{
    if (strcasecmp(type, "f") == 0 || strcasecmp(type, "file") == 0)
    {
        return (S_ISREG(mode));
    }
    else if (strcasecmp(type, "d") == 0 || strcasecmp(type, "dir") == 0 || strcasecmp(type, "directory") == 0)
    {
        return (S_ISDIR(mode));
    }
    else if (strcasecmp(type, "sl") == 0 || strcasecmp(type, "slink") == 0 || strcasecmp(type, "symbolic-link") == 0)
    {
        return (S_ISLNK(mode));
    }

    return false;
}

bool match_extension(char *extensions, char *name)
{
    return;
}

// Checks if size is between min and max
bool match_size(off_t max_size, off_t min_size, off_t size)
{
    if (max_size != 0 && min_size != 0)
    {
        return (size < max_size && size > min_size);
    }
    else if (max_size != 0)
    {
        return (size < max_size);
    }
    else  // min_size != 0
    {
        return (size > min_size);
    }
}
