#ifndef MOVE_H
#define MOVE_H

// Libraries
#include <stdbool.h>

// Headers
#include "cli_opts.h"

// Structure to all CLI flags of 'move' functionality
typedef struct
{
    GeneralOptions base;  // extension | recursive
    FilterOptions filter;  // contains | min_size | max_size | type
    ActionOptions action;  // dry-run | interactive | verbose
    bool force;
    bool skip;
} MoveOptions;

// Prototype
int handle_move(int argc, char **argv);

#endif
