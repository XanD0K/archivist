#ifndef MOVE_H
#define MOVE_H

#include <stdbool.h>

#include "cli_opts.h"

typedef struct
{
    GeneralOptions base;  // extension | recursive
    FilterOptions filter;  // min_size | max_size | type
    char *contains;
    bool dry_run;
    bool force;
    bool interactive;
    bool skip;
    bool verbose;
} MoveOptions;

// Prototype
int handle_move(int argc, char **argv);

#endif
