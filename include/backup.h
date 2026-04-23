#ifndef BACKUP_H
#define BACKUP_H

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Flags structure
typedef struct
{
    CommonOptions base;    // human-readable | recursive
    FilterOptions filter;  // contains | extension | max-size | min-size
    ActionOptions action;  // dry-run | interactive | verbose
} BackupOptions;

// Structure to all counters
typedef struct
{
    size_t ext;
    size_t error;
    size_t bck_files;
} BackupCounters;

// Prototypes
ErrorCode handle_backup(int argc, char **argv, int min_args);
ErrorCode parse_backup_options(int argc, char **argv, int opt_start, void *opts_out);

#endif
