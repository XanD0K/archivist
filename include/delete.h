#ifndef DELETE_H
#define DELETE_H

// Libraries
#include <stddef.h>  // size_t
#include <sys/types.h>  // off_t

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Structure to all CLI flags of 'delete' functionality
typedef struct
{
    CommonOptions base;    // human_readable | recursive
    FilterOptions filter;  // contains | extension | min_size | max_size | type
    ActionOptions action;  // dry-run | interactive | verbose

} DeleteOptions;

// Structure to all counters
typedef struct
{
    size_t ext;
    size_t error;
    size_t dlt_files;
    size_t dlt_directories;
    off_t dlt_size;
} DeleteCounters;

// Prototypes
ErrorCode handle_delete(int argc, char **argv, int min_args);
ErrorCode parse_delete_options(int argc, char **argv, int opt_start, void *opts_out);

#endif
