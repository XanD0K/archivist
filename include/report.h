#ifndef REPORT_H
#define REPORT_H

// Libraries
#include <stddef.h>  // size_t
#include <sys/types.h>  // off_t

// Headers
#include "cli_opts.h"

// Alias of a pointer to a function
typedef int (*SortReport)(const void *a, const void *b);

// Structure to all CLI flags of 'report' functionality
typedef struct
{
    CommonOptions base;  // human-readable | recursive | sort
    FilterOptions filter;  // extension
    
} ReportOptions;

// Prototypes
int handle_report(int argc, char **argv);

#endif
