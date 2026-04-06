#ifndef DELETE_H
#define DELETE_H

#include "cli_opts.h"

// Structure to all CLI flags of 'delete' functionality
typedef struct
{
    GeneralOptions base;  // extension | human_readable | recursive
    FilterOptions filter;  // contains | min_size | max_size | type
    ActionOptions action;  // dry-run | interactive | verbose

} DeleteOptions;

// Prototypes
int handle_delete(int argc, char **argv);

#endif
