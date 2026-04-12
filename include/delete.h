#ifndef DELETE_H
#define DELETE_H

#include "cli_opts.h"

// Structure to all CLI flags of 'delete' functionality
typedef struct
{
    CommonOptions base;  // human_readable | recursive
    FilterOptions filter;  // contains | extension | min_size | max_size | type
    ActionOptions action;  // dry-run | interactive | verbose

} DeleteOptions;

// Prototypes
int handle_delete(int argc, char **argv, int min_args);
int parse_delete_options(int argc, char **argv, int opt_start, void *opts_out);

#endif
