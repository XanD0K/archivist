#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>
#include <sys/types.h>

#include "cli_opts.h"

typedef struct
{
    GeneralOptions base;  // extension | ignore-case | recursive
    bool contains;
    off_t min_size;
    off_t max_size;
    char *type;
} SearchOptions;

// Prototypes
int handle_search(int argc, char **argv);

#endif
