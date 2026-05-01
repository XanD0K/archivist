#ifndef LIST_H
#define LIST_H

// Libraries
#include <stdbool.h>
#include <stddef.h>  // size_t
#include <sys/types.h>  // off_t

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Structure to all CLI flags of 'list' functionality
typedef struct
{
    CommonOptions base;  // human-readable | recursive | sort    
    bool ignore_case;
    bool reverse;
    bool dir_first;
} ListOptions;

// Structure to all counters
typedef struct
{
    size_t files;
    size_t directories;
    size_t slinks;
    size_t error;
    size_t others;
    off_t total_size;
} ListCounters;

// Prototypes
ErrorCode handle_list(int argc, char **argv, int min_args);
ErrorCode parse_list_opts(int argc, char **argv, int opt_start, void *opts_out);

#endif
