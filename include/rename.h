#ifndef RENAME_H
#define RENAME_H

// Libraries
#include <stddef.h>  // size_t

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Flags structure
typedef struct
{
    CommonOptions base;    // recursive | sort
    FilterOptions filter;  // contains | extension | max-size | min-size
    ActionOptions action;  // dry-run | interactive | verbose
    char *name;
} RenameOptions;

// Structure to all counters
typedef struct
{
    size_t ext;
    size_t name;
    size_t rnmd_files;
    size_t error;
} RenameCounters;

// Prototypes
ErrorCode handle_rename(int argc, char **argv, int min_args);
ErrorCode parse_rename_opts(int argc, char **argv, int opt_start, void *opts_out);

#endif
