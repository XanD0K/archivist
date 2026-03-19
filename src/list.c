#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Headers
#include "commands.h"

// Prototypes
static bool check_sort(char *sort);
static int cmp_date(const struct dirent **a, const struct dirent **b);
static int cmp_size(const struct dirent **a, const struct dirent **b);
static int cmp_type(const struct dirent **a, const struct dirent **b);
static int check_is_dir(const struct dirent **a, const struct dirent **b);
static void check_element(struct dirent **namelist);

// Globals
const char *sorts[] = {"date", "name", "size", "type", "version"};
const char* current_base_dir = NULL;

// Lists information from a given directory
int handle_list(int argc, char **argv, struct stat *st, DIR *dir)
{
    size_t f_counter = 0, dir_counter = 0, slink_counter = 0, total_size = 0;

    current_base_dir = argv[2];

    SortFunc sorter = alphasort;
    if (argc == 4)
    {
        // Checks for valid sort method
        if (!check_sort(argv[3]))
        {
            errno = EINVAL;
            fprintf(stderr, "Invalid sort argument: %s\n"
                            "Usage: ./archivis list DIRECTORY [name/version/type/size/date]", strerror(errno));
            return 7;
        }

        // Gets sort function for given sort method
        sorter = (strcasecmp(argv[3], "name") == 0) ? sorter : get_sort_func(argv[3]);
    }

    // Gets all elements in directory
    struct dirent **namelist;
    int n = scandir(current_base_dir, &namelist, NULL, sorter);

    if (n == -1)
    {
        perror("scandir");
        return 8;
    }

    for (int i = 0; i < n; i++)
    {
        // Prints all elements in directory
        printf("%s\n", namelist[i]->d_name);
        // Updates f_counter, dir_counter and total_size variables
        check_element(namelist[i]);
        free(namelist[i]);
    }

    // PRINTS f_counter, dir_counter and total_size variables

    free(namelist);
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
    return alphasort;
}


static int cmp_date(const struct dirent **a, const struct dirent **b)
{
    int isDir = check_is_dir(*a, *b);
    if (isDir != 0)
    {
        return isDir;
    }

    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    // Constructs full path for current elements
    snprintf(pathA, sizeof(pathA), "%s/%s", current_base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", current_base_dir, (*b)->d_name); 

    // Gets element's data
    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (strcasecmp((*a)->d_name, (*b)->d_name));
    }
    
    return (sa.st_mtime > sb.st_mtime) - (sa.st_mtime < sb.st_mtime);
}

static int cmp_size(const struct dirent **a, const struct dirent **b)
{
    int isDir = check_is_dir(*a, *b);
    if (isDir != 0)
    {
        return isDir;
    }

    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    snprintf(pathA, sizeof(pathA), "%s/%s", current_base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", current_base_dir, (*b)->d_name);

    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (strcasecmp((*a)->d_name, (*b)->d_name));
    }

    return (sa.st_size > sb.st_size) - (sa.st_size < sb.st_size);
}

static int cmp_type(const struct dirent **a, const struct dirent **b)
{
    int isDir = check_is_dir(*a, *b);
    if (isDir != 0)
    {
        return isDir;
    }

    const char *extA = strrchr((*a)->d_name, '.');
    const char *extB = strrchr((*b)->d_name, '.');

    if (extA == NULL && extB != NULL)
    {
        return -1;
    }
    if (extA != NULL && extB == NULL)
    {
        return 1;
    }

    if (extA == NULL)
    {
        extA = "";
    }

    if (extB == NULL)
    {
        extB = "";
    }

    return strcasecmp(extA, extB);
}

// Helper function that puts directories before files
static int check_is_dir(const struct dirent **a, const struct dirent **b)
{
    int isDirA = ((*a)->d_type == DT_DIR);
    int isDirB = ((*b)->d_type == DT_DIR);

    // Folders before files
    if (isDirA != isDirB)
    {
        return isDirB - isDirA;
    }

    return 0;
}

// Traverse through every element and updates counters
static void check_element(struct dirent **namelist)
{

}