#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <limits.h>  // PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// Headers
#include "utils.h"
#include "utils_filter.h"

bool check_directory_flags(const struct dirent *namelist, const char *full_path,
                           const FilterOptions *filter, ScandirFilter scandir_filter)
{
    if (!filter)
    {
        return true;
    }

    // Checks directory's type (-t|--type)
    if ((filter->supported & FILTER_TYPE) &&
        filter->type && filter->type[0] != '\0')
    {
        if (is_directory_type(filter->type))
        {
            // Checks for match with searched name (-c|--contains)
            if ((filter->supported & FILTER_CONTAINS) &&
                filter->contains && filter->contains[0] != '\0')
            {
                if (!match_name(namelist->d_name, filter->contains))
                {
                    return false;
                }
            }

            // Checks for directory's size (--max-size|--min-size)
            if ((filter->supported & (FILTER_MAX_SIZE | FILTER_MIN_SIZE)) &&
                (filter->max_size || filter->min_size))
            {
                off_t dir_size = 0;
                if (!match_directory_size(full_path, filter->max_size, filter->min_size,
                                        &dir_size, scandir_filter))
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    return false;
}

bool check_file_flags(const struct dirent *namelist, Extension *ext, const char *full_path,
                      const FilterOptions *filter, size_t ext_counter, size_t *err_counter)
{
    if (!filter)
    {
        return true;
    }

    // Checks for match with searched name (-c|--contains)
    if ((filter->supported & FILTER_CONTAINS) &&
        filter->contains && filter->contains[0] != '\0')
    {
        if (!match_name(namelist->d_name, filter->contains))
        {
            return false;
        }
    }

    // Checks file's extension (-e|--extension)
    if ((filter->supported & FILTER_EXTENSION) &&
        filter->extension && filter->extension[0] != '\0')
    {
        if (!match_extension(ext, ext_counter, namelist->d_name))
        {
            return false;
        }
    }

    if ((filter->supported & (FILTER_MAX_SIZE | FILTER_MIN_SIZE | FILTER_TYPE)) &&
        (filter->type || filter->max_size || filter->min_size))
    {
        struct stat st;
        if (lstat(full_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
            (*err_counter)++;
            return false;
        }

        // Checks for element's type (-t|--type)
        if ((filter->supported & FILTER_TYPE) && filter->type && filter->type[0] != '\0')
        {
            if (!match_type(filter->type, st.st_mode))
            {
                return false;
            }
        }

        // Checks for file's size (--max-size|--min-size)
        if ((filter->supported & (FILTER_MAX_SIZE | FILTER_MIN_SIZE)) &&
            (filter->max_size || filter->min_size))
        {
            if (!match_size(filter->max_size, filter->min_size, st.st_size))
            {
                return false;
            }
        }
    }

    return true;
}

// Checks if name matches
bool match_name(const char *name, const char *contains)
{
    return (strcasestr(name, contains) != NULL);
}

// Checks if type matches
bool match_type(const char *type, mode_t mode)
{
    if (is_file_type(type))
    {
        return (S_ISREG(mode));
    }
    else if (is_directory_type(type))
    {
        return (S_ISDIR(mode));
    }
    else if (is_slink_type(type))
    {
        return (S_ISLNK(mode));
    }

    return false;
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
bool match_directory_size(const char *path, off_t max_size, off_t min_size,
                          off_t *total_size, ScandirFilter filter)
{
    *total_size = 0;

    struct dirent **namelist;
    int n = scandir(path, &namelist, filter, alphasort);
    if (n == -1)
    {
        return false;
    }

    for (int i = 0; i < n; i++)
    {
        char new_path[PATH_MAX];
        if (check_path_name_size(new_path, sizeof(new_path), path, namelist[i]->d_name) == -1)
        {
            free_dirent(namelist, n);
            return false;
        }

        struct stat st;
        if (stat(new_path, &st) != 0)
        {
            free_dirent(namelist, n);
            return false;
        }

        if(S_ISDIR(st.st_mode))
        {
            off_t subdir_size = 0;
            if (!match_directory_size(new_path, max_size, min_size, &subdir_size, filter))
            {
                free_dirent(namelist, n);
                return false;
            }

            *total_size += subdir_size;
        }
        else
        {
            *total_size += st.st_size;
        }

        // Early exit
        if ((max_size > 0 && *total_size > max_size) ||
            (min_size > 0 && *total_size >= min_size))
            {
                break;
            }
    }

    free_dirent(namelist, n);
    return match_size(max_size, min_size, *total_size);
}

// Checks if extension matches
bool match_extension(Extension *exts, size_t ext_counter, const char *name)
{
    const char *ext_name = get_clean_extension(name);
    if (!ext_name || ext_name[0] == '\0')
    {
        return false;
    }

    for (size_t i = 0; i < ext_counter; i++)
    {
        const char *clean_ext = exts[i].extension;
        
        if (clean_ext && clean_ext[0] == '.')
        {
            clean_ext = clean_ext + 1;
        }
        if (strcasecmp(ext_name, clean_ext) == 0)
        {
            return true;
        }
    }

    return false;
}
