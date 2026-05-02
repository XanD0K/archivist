#ifndef CLI_OPTS_H
#define CLI_OPTS_H

#include <stdbool.h>
#include <stdint.h>  // uint32_t
#include <sys/types.h>  // off_t

// ============ COMMON FLAGS ============
typedef struct
{
    bool all;
    bool almost_all;
    bool human_readable;
    bool recursive;
    char *sort;
} CommonOptions;
// Bitmask for CommonOptions
#define COMMON_ALL (1U << 0)
#define COMMON_ALMOST_ALL (1U << 1)
#define COMMON_HUMAN_READABLE (1U << 2)
#define COMMON_RECURSIVE (1U << 3)
#define COMMON_SORT (1U << 4)

// ============ FILTER FLAGS ============
typedef struct
{
    char *contains;
    char *extension;
    off_t max_size;
    off_t min_size;
    char *type;
    uint32_t supported;  // All supported flags for this structure
} FilterOptions;
// Bitmask for FilterOptions
#define FILTER_CONTAINS (1U << 8)
#define FILTER_EXTENSION (1U << 9)
#define FILTER_MAX_SIZE (1U << 10)
#define FILTER_MIN_SIZE (1U << 11)
#define FILTER_TYPE (1U << 12)

// ============ ACTION FLAGS ============
typedef struct
{
    bool dry_run;
    bool interactive;
    bool verbose;
} ActionOptions;
// Bitmask for ActionOptions
#define ACTION_DRY_RUN (1U << 16)
#define ACTION_INTERACTIVE (1U << 17)
#define ACTION_VERBOSE (1U << 18)

#endif
