#ifndef UTILS_SORT_H
#define UTILS_SORT_H

// Libraries
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>  // size_t

// Type/Alias
typedef int (*SortFlag)(const struct dirent **a, const struct dirent **b);

// Structure for modifiable global variables used by 'sort' flag
typedef struct{
    int reverse;
    bool dir_first;
    bool ignore_case;
    char *base_dir;
} CompareOptions;

extern CompareOptions cmp_opts;

// Prototypes
SortFlag get_sort_function(char *sort, const char **sorts, size_t len);
static int cmp_version_scandir(const struct dirent **a, const struct dirent **b);
static int cmp_name_scandir(const struct dirent **a, const struct dirent **b);
static int cmp_date_scandir(const struct dirent **a, const struct dirent **b);
static int cmp_size_scandir(const struct dirent **a, const struct dirent **b);
static int cmp_ext_scandir(const struct dirent **a, const struct dirent **b);
static int check_is_dir(const struct dirent *a, const struct dirent *b);

#endif
