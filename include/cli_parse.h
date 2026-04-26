#ifndef CLI_PARSE_COMMON_H
#define CLI_PARSE_COMMON_H

// Libraries
#include <stdint.h>  // uint32_t

// Headers
#include "cli_opts.h"
#include "error_code.h"

// Prototypes
void handle_common_flag(int opt, char *opt_arg, CommonOptions *opts,
                         uint32_t supported_flags);
void handle_filter_flag(int opt, const char *opt_name, char *opt_arg,
                         FilterOptions *opts, uint32_t supported_flags);
void handle_action_flag(int opt, ActionOptions *opts, uint32_t supported_flags);

#endif
