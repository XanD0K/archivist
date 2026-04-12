#define _GNU_SOURCE

// Libraries
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// Headers
#include "utils.h"
#include "utils_filter.h"

// Checks if name matches
bool match_name(char *contains, const char *name)
{
    return (strcasestr(name, contains) != NULL);
}

// Checks if type matches
bool match_type(const char *type, mode_t mode)
{
    if (strcasecmp(type, "f") == 0 || strcasecmp(type, "file") == 0)
    {
        return (S_ISREG(mode));
    }
    else if (is_directory_type(type))
    {
        return (S_ISDIR(mode));
    }
    else if (strcasecmp(type, "sl") == 0 || strcasecmp(type, "slink") == 0 || strcasecmp(type, "symbolic-link") == 0)
    {
        return (S_ISLNK(mode));
    }

    return false;
}

bool is_directory_type (const char *type)
{
    return (strcasecmp(type, "d") == 0 ||
            strcasecmp(type, "dir") == 0 ||
            strcasecmp(type, "directory") == 0);
}

// Checks if file's size is between min and max
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

// Checks if directory's size is between min and max
bool match_directory_size(const char *path, off_t max_size, off_t min_size , off_t *total_size)
{
    if (*total_size == 0 && (max_size > 0 || min_size > 0))
    {
        *total_size = 0;
    }

    bool result = false;

    struct dirent **namelist;
    int n = scandir(path, &namelist, NULL, alphasort);
    if (n == -1)
    {
        return false;
    }

    for (int i = 0; i < n; i++)
    {
        char new_path[PATH_MAX];
        if (check_path_name_size(new_path, sizeof(new_path), path, namelist[i]->d_name) == -1)
        {
            goto cleanup;
        }

        struct stat st;
        if (stat(new_path, &st) != 0)
        {
            goto cleanup;
        }

        if(S_ISDIR(st.st_mode))
        {
            if (!match_directory_size(new_path, max_size, min_size, total_size))
            {
                goto cleanup;
            }
        }
        else
        {
            *total_size += st.st_size;
        }

        // Early exit
        if ((max_size > 0 && *total_size > max_size) ||
            (min_size > 0 && *total_size >= min_size))
            {
                goto cleanup;
            }

        free(namelist[i]);
    }

    free(namelist);

    result = match_size(max_size, min_size, *total_size);    

cleanup:
    for (int i = 0; i < n; i++)
    {
        free(namelist[i]);
    }
    free(namelist);
    return result;
}

// Checks if extension matches
bool match_extension(Extension *exts, size_t ext_counter, char *name)
{
    const char *ext_name = get_clean_extension(name);
    if (!ext_name || ext_name[0] == '\0')
    {
        return false;
    }

    for (size_t i = 0; i < ext_counter; i++)
    {
        const char *clean_ext = exts[i].extension;
        
        if (clean_ext[0] == '.')
        {
            clean_ext++;
        }
        if (strcasecmp(ext_name, clean_ext) == 0)
        {
            return true;
        }
    }

    return false;
}
