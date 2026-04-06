#ifndef CLI_OPTS_H
#define CLI_OPTS_H

#include <stdbool.h>
#include <sys/types.h>  // off_t

// General flags shared across files
typedef struct
{
    char *extension;
    bool human_readable;
    bool ignore_case;
    bool recursive;
    char *sort;
} GeneralOptions;

// Filter flags shared across files
typedef struct
{
    char *contains;
    off_t min_size;
    off_t max_size;
    char *type;
} FilterOptions;

// Action flags shared across files
typedef struct
{
    bool dry_run;
    bool interactive;
    bool verbose;
} ActionOptions;

#endif
