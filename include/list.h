#ifndef LIST_H
#define LIST_H

// Libraries
#include <dirent.h>
#include <stdbool.h>

// Headers
#include "cli_opts.h"

// Alias of a pointer to a function
typedef int (*SortList)(const struct dirent **a, const struct dirent **b);

// Structure to all CLI flags of 'list' functionality
typedef struct
{
    CommonOptions base;  // human-readable | ignore-case | recursive | sort    
    bool reverse;
    bool dir_first;
} ListOptions;

// Prototypes
int handle_list(int argc, char **argv);

#endif
