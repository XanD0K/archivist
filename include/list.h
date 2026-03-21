#ifndef LIST_H
#define LIST_H

#include <dirent.h>
#include <stdbool.h>

// Type/Alias
typedef int (*SortFunc)(const struct dirent **a, const struct dirent **b);

// Structure to all CLI flags of 'list' functionality
typedef struct
{
    char *order;
    bool reverse;
    bool recursive;
    bool dir_first;
    bool case_sensitive;
} ListOptions;

// Prototypes
int handle_list(int argc, char **argv);

#endif
