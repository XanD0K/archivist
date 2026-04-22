#ifndef REPORT_H
#define REPORT_H

// Libraries
#include <stddef.h>  // size_t
#include <sys/types.h>  // off_t | ssize_t

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Alias of a pointer to a function
typedef int (*SortReport)(const void *a, const void *b);

// Structure to all CLI flags of 'report' functionality
typedef struct
{
    CommonOptions base;    // human-readable | recursive | sort
    FilterOptions filter;  // extension    
} ReportOptions;

// Structure to all counters
typedef struct
{
    size_t ext;
    size_t ext_capacity;
    size_t user_ext;
    size_t total_files;
    off_t total_size;
    size_t error;
} ReportCounters;

// Prototypes
ErrorCode handle_report(int argc, char **argv, int min_args);
ErrorCode parse_report_opts(int argc, char **argv, int opt_start, void *opts_out);

#endif
