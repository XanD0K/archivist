#define _GNU_SOURCE

// Libraries]
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Headers
#include "commands.h"

// Prototypes
static bool check_sort(char *sort);
static int cmp_name(const struct dirent **a, const struct dirent **b);
static int cmp_date(const struct dirent **a, const struct dirent **b);
static int cmp_size(const struct dirent **a, const struct dirent **b);
static int cmp_type(const struct dirent **a, const struct dirent **b);
static int check_is_dir(const struct dirent **a, const struct dirent **b);
static void check_element(struct dirent **namelist, const char *current_path, size_t *f_counter,
                          size_t *dir_counter, size_t *slink_counter, size_t *err_counter,
                          size_t *total_size, SortFunc sorter, bool recursive);

// Globals
static const char *sorts[] = {"date", "name", "size", "type", "version"};
char *base_dir = NULL;
static int order;

// Lists information from a given directory
int handle_list(int argc, char **argv)
{
    size_t f_counter = 0, dir_counter = 0, slink_counter = 0, err_counter = 0, total_size = 0;

    base_dir = malloc(strlen(argv[2]) + 1);
    if (base_dir == NULL)
    {
        return 6;
    }
    strcpy(base_dir, argv[2]);
    // Ensures path has no trailing /
    size_t base_dir_len = strlen(base_dir);
    if (base_dir_len > 0 && base_dir[base_dir_len - 1] == '/')
    {
        base_dir[base_dir_len - 1] = '\0';
    }

    order = 1;
    if (argc >= 5)
    {
        order = (strcasecmp(argv[4], "asc") == 0) ? order : -1;
    }

    bool recursive = false;
    if (argc == 6)
    {
        recursive = (strcasecmp(argv[5], "recursive") == 0) ? recursive : true;
    }

    SortFunc sorter = cmp_name;
    if (argc >= 4)
    {
        // Checks for valid sort method
        if (!check_sort(argv[3]))
        {
            errno = EINVAL;
            fprintf(stderr, "Invalid sort argument: %s\n"
                            , strerror(errno));
            return 7;
        }

        // Gets sort function for given sort method
        sorter = (strcasecmp(argv[3], "name") == 0) ? sorter : get_sort_func(argv[3]);
    }

    // Gets all elements in directory
    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, sorter);

    if (n == -1)
    {
        perror("scandir");
        return 8;
    }

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            if (!recursive)
            {
                // Prints file's name
                printf("%s\n", namelist[i]->d_name);
            }            
            // Recursively checks directory elements and updates counters
            check_element(namelist[i], base_dir, &f_counter, &dir_counter, &slink_counter,
                          &err_counter, &total_size, sorter, recursive);
            free(namelist[i]);
        }
    }

    free(namelist);
    free(base_dir);

    // Prints f_counter, dir_counter and total_size variables
    printf("Directories: %zu\n"
           "Files: %zu\n"
           "Simbolic Links: %zu\n"
           "Total size: %zu\n", dir_counter, f_counter, slink_counter, total_size);
    if (err_counter != 0)
    {
        printf("(Finish listing with %zu erros)\n", err_counter);
        return 9;
    }
    
    return 0;
}

// Checks for valid sort method
static bool check_sort(char *sort)
{
    size_t sorts_len = sizeof(sorts) / sizeof(sorts[0]);

    for (int i = 0; i < sorts_len; i++)
    {
        if (strcasecmp(sort, sorts[i]) == 0)
        {
            return true;
        }
    }

    return false;
}


// Returns specific sort function based on chosen sort method
SortFunc *get_sort_func(char *sort)
{
    if (strcasecmp(sort, "version") == 0)
    {
        return versionsort;
    }

    else if (strcasecmp(sort, "date") == 0)
    {
        return cmp_date;
    }

    else if (strcasecmp(sort, "size") == 0)
    {
        return cmp_size;
    }

    else if (strcasecmp(sort, "type") == 0)
    {
        return cmp_type;
    }

    // Fallback
    return cmp_name;
}

static int cmp_name(const struct dirent **a, const struct dirent **b)
{
    int result = (strcasecmp((*a)->d_name, (*b)->d_name));

    return result * order;
}

static int cmp_date(const struct dirent **a, const struct dirent **b)
{
    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    // Constructs full path for current elements
    snprintf(pathA, sizeof(pathA), "%s/%s", base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", base_dir, (*b)->d_name); 

    // Gets element's data
    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (strcasecmp((*a)->d_name, (*b)->d_name));
    }
    
    int result = (sa.st_mtime > sb.st_mtime) - (sa.st_mtime < sb.st_mtime);
    
    return result * order;
}

static int cmp_size(const struct dirent **a, const struct dirent **b)
{
    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    snprintf(pathA, sizeof(pathA), "%s/%s", base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", base_dir, (*b)->d_name);

    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (strcasecmp((*a)->d_name, (*b)->d_name));
    }

    int result = (sa.st_size > sb.st_size) - (sa.st_size < sb.st_size);
    
    return result * order;

}

static int cmp_type(const struct dirent **a, const struct dirent **b)
{
    const char *extA = strrchr((*a)->d_name, '.');
    const char *extB = strrchr((*b)->d_name, '.');

    if (extA == NULL)
    {
        extA = "";
    }

    if (extB == NULL)
    {
        extB = "";
    }

    int result = strcasecmp(extA, extB);

    return result * order;
}


// Traverses through every element and updates counters
static void check_element(struct dirent **namelist, const char *current_path, size_t *f_counter,
                          size_t *dir_counter, size_t *slink_counter, size_t *err_counter,
                          size_t *total_size, SortFunc sorter, bool recursive)
{
    if (strcmp((*namelist)->d_name, ".") == 0 || strcmp((*namelist)->d_name, "..") == 0)
    {
        return;
    }
    
    struct stat st;
    char new_path[PATH_MAX];
    snprintf(new_path, sizeof(new_path), "%s/%s", current_path, (*namelist)->d_name);
    
    if (stat(new_path, &st) != 0)
    {
        fprintf(stderr, "Couldn't access %s: %s\n", new_path, strerror(errno));
        *err_counter++;
        return;
    }

    // File
    if (S_ISREG(st.st_mode))
    {
        *f_counter++;
        *total_size += st.st_size;
    }
    // Simbolic Link
    else if (S_ISLNK(st.st_mode))
    {
        *slink_counter++;
    }
    // Directory
    else if (S_ISDIR(st.st_mode))
    {
        *dir_counter++;

        struct dirent **entry;
        int n = scandir(new_path, &entry, NULL, sorter);

        if (n == -1)
        {
            perror("scandir");
            *err_counter++;
        }

        for (int i = 0; i < n; i++)
        {
            // Recursively checks directory elements and updates counters
            check_element(entry[i], new_path, &f_counter, &dir_counter, &slink_counter,
                          &err_counter, &total_size, sorter, recursive);
            free(entry[i]);
        }

        free(entry);
    }
    
    if (recursive)
    {
        // Prints file's name
        const char *sufix = new_path + strlen(base_dir);
        printf("%s\n", sufix);   
    }

    return;
}