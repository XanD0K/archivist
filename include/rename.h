#ifndef RENAME_H
#define RENAME_H

// Libraries
#include <dirent.h>

// Headers
#include "cli_opts.h"
#include "error_code.h"

typedef struct
{
    CommonOptions base;  // recursive | sort
    FilterOptions filter;  // contains | extension | max-size | min-size
    ActionOptions action;  // dry-run | interactive | verbose
    char *name;
} RenameOptions;

// Prototypes
ErrorCode handle_rename(int argc, char **argv, int min_args);
ErrorCode parse_rename_options(int argc, char **argv, int opt_start, void *opts_out);

#endif