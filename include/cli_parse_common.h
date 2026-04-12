#ifndef CLI_PARSE_COMMON_H
#define CLI_PARSE_COMMON_H

// Headers
#include "cli_opts.h"

// GLOBAL
#define PARSE_ERROR_INVALID_SIZE 2

// Prototypes
int parse_common_opts(int argc, char **argv, int opt_start, CommonOptions *opts);
int parse_filter_options(int argc, char **argv, int opt_start, FilterOptions *opts);
int parse_action_options(int argc, char **argv, int opt_start, ActionOptions *opts);

#endif
