#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>

#include "cli_opts.h"

// Structure to all CLI flags of 'search' functionality
typedef struct
{
    CommonOptions base;  // ignore-case | recursive
    FilterOptions filter;  // extension | min_size | max_size | type
    bool contains;
} SearchOptions;

// Prototypes
int handle_search(int argc, char **argv);

#endif
