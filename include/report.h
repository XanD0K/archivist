#ifndef REPORT_H
#define REPORT_H

// Libraries
#include <stddef.h>
#include <sys/types.h>

// Headers
#include "cli_opts.h"

typedef struct
{
    GeneralOptions base;  // extension | human-readable | recursive
    
} ReportOptions;

typedef struct Extension
{
    char *extension;
    off_t size;
    size_t file_count;
} Extension;

#endif
