#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>

#include "cli_opts.h"

// Structure to all CLI flags of 'search' functionality
typedef struct
{
    GeneralOptions base;  // extension | ignore-case | recursive
    FilterOptions filter;  // min_size | max_size | type
    bool contains;
} SearchOptions;

// Prototypes
int handle_search(int argc, char **argv);

#endif
