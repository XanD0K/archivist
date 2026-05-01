#define _GNU_SOURCE

// Libraries
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// Headers
#include "utils.h"
#include "utils_sort.h"

// Globals
CompareOptions cmp_opts =
{
    .dir_first = false,
    .ignore_case = false,
    .reverse = 1,
    .base_dir = NULL
};

// Returns sort function for scandir()
SortScandir get_scandir_sort_fn(const char *sort, const char **sorts, size_t len)
{
    if (!check_value_in_list(sort, sorts, len))
    {
        errno = EINVAL;
        fprintf(stderr, "Invalid sort argument: %s\n", strerror(errno));
        return NULL;
    }

    if (strcasecmp(sort, "version") == 0)
    {
        return cmp_version_scandir;
    }
    else if (strcasecmp(sort, "date") == 0)
    {
        return cmp_date_scandir;
    }
    else if (strcasecmp(sort, "size") == 0)
    {
        return cmp_size_scandir;
    }
    else if (strcasecmp(sort, "extension") == 0)
    {
        return cmp_ext_scandir;
    }

    // Fallback
    return cmp_name_scandir;
}

// Returns sort function for qsort()
SortQsort get_qsort_sort_fn(const char *sort, const char **sorts, size_t len)
{
    if (!check_value_in_list(sort, sorts, len))
    {
        errno = EINVAL;
        fprintf(stderr, "Invalid sort argument: %s\n", strerror(errno));
        return NULL;
    }

    if (strcasecmp(sort, "size") == 0)
    {
        return cmp_size_qsort;
    }
    else if (strcasecmp(sort, "quantity") == 0)
    {
        return cmp_quantity_qsort;
    }

    // Fallback
    return cmp_name_qsort;
}

// ======================== SCANDIR ========================
// Organizes by name
int cmp_name_scandir(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (cmp_opts.dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir * cmp_opts.reverse;
        }
    }

    int result = (!cmp_opts.ignore_case)
        ? (strcmp((*a)->d_name, (*b)->d_name))
        : (strcasecmp((*a)->d_name, (*b)->d_name)); 

    return result * cmp_opts.reverse;
}

// Organizes by version (distinguish alphabetical and numerical characters)
int cmp_version_scandir(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (cmp_opts.dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir * cmp_opts.reverse;
        }
    }

    // Same strings
    int outter_result = (!cmp_opts.ignore_case)
        ? (strcmp((*a)->d_name, (*b)->d_name))
        : (strcasecmp((*a)->d_name, (*b)->d_name));
    if (outter_result == 0)
    {
        return outter_result;
    }

    // Points to the beggining of each word
    const char *pA = (*a)->d_name;
    const char *pB = (*b)->d_name;

    while (*pA || *pB)
    {
        // Jumps the identical part of earch word
        while (*pA && *pB)
        {
            if (isdigit(*pA) != 0 || isdigit(*pB) != 0)
            {
                break;
            }

            if (cmp_opts.ignore_case)
            {
                if (toupper((unsigned char)*pA) != toupper((unsigned char)*pB))
                {
                    break;
                }
            }
            else
            {
                if(*pA != *pB)
                {
                    break;
                }
            }

            pA++;
            pB++;
        }

        // Checks if one string ended before the other
        if (!*pA || !*pB)
        {
            return (!*pA ? -1 : 1) * cmp_opts.reverse;
        }

        // Checks if character is digit
        bool is_digitA = isdigit((unsigned char)*pA);
        bool is_digitB = isdigit((unsigned char)*pB);

        // Both characters are digits
        if(is_digitA && is_digitB)
        {
            char *endA, *endB;
            unsigned long numA = strtoul(pA, &endA, 10);
            unsigned long numB = strtoul(pB, &endB, 10);

            // Numbers are different
            if (numA != numB)
            {
                return ((numA > numB) ? 1 : -1) * cmp_opts.reverse;
            }

            // Jumps to the string after numbers
            pA = endA;
            pB = endB;
            continue;
        }

        if (is_digitA)
        {
            return -1 * cmp_opts.reverse;
        }
        if (is_digitB)
        {
            return 1 * cmp_opts.reverse;
        }

        // Compares 
        int inner_result = (cmp_opts.ignore_case)
            ? tolower((unsigned char)*pA) - tolower((unsigned char)*pB)
            : (unsigned char)*pA - (unsigned char)*pB;

        if (inner_result != 0)
        {
            return inner_result * cmp_opts.reverse;
        }

        pA++;
        pB++;        
    }

    return 0;
}

// Organizes by date
int cmp_date_scandir(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (cmp_opts.dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir * cmp_opts.reverse;
        }
    }

    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    // Constructs full path for current elements
    snprintf(pathA, sizeof(pathA), "%s/%s", cmp_opts.base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", cmp_opts.base_dir, (*b)->d_name); 

    // Gets element's data
    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (!cmp_opts.ignore_case) ? (strcmp((*a)->d_name, (*b)->d_name)) :(strcasecmp((*a)->d_name, (*b)->d_name));
    }
    
    int result = (sa.st_mtime > sb.st_mtime) - (sa.st_mtime < sb.st_mtime);
    
    return result * cmp_opts.reverse;
}

// Organizes by size
int cmp_size_scandir(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (cmp_opts.dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir * cmp_opts.reverse;
        }
    }

    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    snprintf(pathA, sizeof(pathA), "%s/%s", cmp_opts.base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", cmp_opts.base_dir, (*b)->d_name);

    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (strcasecmp((*a)->d_name, (*b)->d_name));
    }

    int result = (sa.st_size > sb.st_size) - (sa.st_size < sb.st_size);
    
    return result * cmp_opts.reverse;

}

// Organizes by extension
int cmp_ext_scandir(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (cmp_opts.dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir * cmp_opts.reverse;
        }
    }

    const char *extA = get_clean_extension((*a)->d_name);
    const char *extB = get_clean_extension((*b)->d_name);

    int result = strcasecmp(extA, extB);

    return result * cmp_opts.reverse;
}

// Helper function that puts directories before files
int check_is_dir(const struct dirent *a, const struct dirent *b)
{
    int is_dir_a = (a->d_type == DT_DIR);
    int is_dir_b = (b->d_type == DT_DIR);

    // Folders before files
    if (is_dir_a != is_dir_b)
    {
        int result = is_dir_b - is_dir_a;
        return result * cmp_opts.reverse;
    }

    return 0;
}

// ========================= QSORT =========================
// Sorts by name
int cmp_name_qsort(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    if (strcasecmp(extA->extension, "others") == 0)
    {
        return 1;
    }
    if (strcasecmp(extB->extension, "others") == 0)
    {
        return -1;
    }

    return (strcasecmp(extA->extension, extB->extension));
}

// Sorts by size
int cmp_size_qsort(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    if (extA->size > extB->size) 
    {
        return 1;
    }
    else if (extA->size < extB->size)
    {
        return -1;
    }
    return 0;
}

// Sorts by quantity
int cmp_quantity_qsort(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    return ((int)extB->file_count - (int)extA->file_count);
}
