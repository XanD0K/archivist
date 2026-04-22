#ifndef MOVE_H
#define MOVE_H

// Libraries
#include <stdbool.h>
#include <stddef.h>  // size_t | ptrdiff_t

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Structure to all CLI flags of 'move' functionality
typedef struct
{
    CommonOptions base;    // recursive
    FilterOptions filter;  // contains | extension | max_size | min_size | type
    ActionOptions action;  // dry-run | interactive | verbose
    bool force;
    bool skip;
} MoveOptions;

// Structure to all counters
typedef struct
{
    size_t ext;
    size_t error;
    size_t moved_directories;
    size_t moved_files;
} MoveCounters;

// Prototype
ErrorCode handle_move(int argc, char **argv, int min_args);
ErrorCode parse_move_opts(int argc, char **argv, int opt_start, void *opts_out);

#endif
