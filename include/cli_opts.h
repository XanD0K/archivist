#ifndef CLI_OPTS_H
#define CLI_OPTS_H

#include <stdbool.h>
#include <sys/types.h>  // off_t

// ============ COMMON FLAGS ============
typedef struct
{
    bool human_readable;
    bool ignore_case;
    bool recursive;
    char *sort;
} CommonOptions;
// Bitmask for CommonOptions
#define COMMON_HUMAN_READABLE (1U << 0)
#define COMMON_IGNORE_CASE (1U << 1)
#define COMMON_RECURSIVE (1U << 2)
#define COMMON_SORT (1U << 3)

// ============ FILTER FLAGS ============
typedef struct
{
    char *contains;
    char *extension;
    off_t max_size;
    off_t min_size;
    char *type;
} FilterOptions;
// Bitmask for FilterOptions
#define FILTER_CONTAINS (1U << 0)
#define FILTER_EXTENSION (1U << 1)
#define FILTER_MAX_SIZE (1U << 2)
#define FILTER_MIN_SIZE (1U << 3)
#define FILTER_TYPE (1U << 4)

// ============ ACTION FLAGS ============
typedef struct
{
    bool dry_run;
    bool interactive;
    bool verbose;
} ActionOptions;
// Bitmask for ActionOptions
#define ACTION_DRY_RUN (1U << 0)
#define ACTION_INTERACTIVE (1U << 1)
#define ACTION_VERBOSE (1U << 2)

#endif
