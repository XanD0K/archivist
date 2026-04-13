#ifndef CLI_PARSE_COMMON_H
#define CLI_PARSE_COMMON_H

// Libraries
#include <stdint.h>  // uint32_t

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Prototypes
ErrorCode parse_common_opts(int argc, char **argv, int opt_start, CommonOptions *opts, uint32_t supported_flags);
ErrorCode parse_filter_options(int argc, char **argv, int opt_start, FilterOptions *opts, uint32_t supported_flags);
ErrorCode parse_action_options(int argc, char **argv, int opt_start, ActionOptions *opts, uint32_t supported_flags);

#endif
