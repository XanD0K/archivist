#ifndef LIST_H
#define LIST_H

// Libraries
#include <dirent.h>
#include <stdbool.h>

// Headers
#include "cli_opts.h"

// Structure to all CLI flags of 'list' functionality
typedef struct
{
    CommonOptions base;  // human-readable | ignore-case | recursive | sort    
    bool reverse;
    bool dir_first;
} ListOptions;

// Prototypes
int handle_list(int argc, char **argv, int min_args);
int parse_list_opts(int argc, char **argv, int opt_start, void *opts_out);

#endif
