#ifndef RENAME_H
#define RENAME_H

// Libraries
#include <dirent.h>

// Headers
#include "cli_opts.h"

typedef struct
{
    CommonOptions base;  // recursive | sort
    FilterOptions filter;  // contains | extension | max-size | min-size | type
    ActionOptions action;  // dry-run | interactive | verbose
    char *name;
} RenameOptions;

// Prototypes
int handle_rename(int argc, char **argv, int min_args);

#endif