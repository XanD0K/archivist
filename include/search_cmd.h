#ifndef SEARCH_H
#define SEARCH_H

// Libraries
#include <stdbool.h>

// headers
#include "cli_opts.h"
#include "error_code.h"

// Structure to all CLI flags of 'search' functionality
typedef struct
{
    CommonOptions base;  // ignore-case | recursive
    FilterOptions filter;  // extension | max_size | min_size | type
    bool contains;
} SearchOptions;

// Prototypes
ErrorCode handle_search(int argc, char **argv, int min_args);
ErrorCode parse_search_opts(int argc, char **argv, int opt_start, void *opts_out);

#endif
