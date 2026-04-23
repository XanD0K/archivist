#ifndef RECOVER_H
#define RECOVER_H

// Libraries
#include <stdbool.h>

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Flags structure
typedef struct
{
    CommonOptions base;    // human-readable
    ActionOptions action;  // dry-run | interactive | verbose
} RecoverOptions;

// Structure to all counters
typedef struct
{
    size_t error;
    size_t rcv_files;
} RecoverCounters;

// Prototypes
ErrorCode handle_recover(int argc, char **argv, int min_args);
ErrorCode parse_recover_options(int argc, char **argv, int opt_start, void *opts_out);

#endif
