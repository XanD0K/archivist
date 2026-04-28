// Libraries
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>  // NULL
#include <stdio.h>
#include <string.h>
#include <unistd.h>  // optind | optarg

// Headers
#include "cli_parse.h"
#include "utils.h"

void handle_common_flag(int opt, char *opt_arg, CommonOptions *opts,
                         uint32_t supported_flags)
{
    switch (opt)
    {
        case 'a': // all
        {
            if(supported_flags & COMMON_ALL)
            {
                opts->all = true;
            }
            break;
        }
        case 'h':  // human-readable
        {
            if (supported_flags & COMMON_HUMAN_READABLE)
            {
                opts->human_readable = true;
            }
            break;
        }
        case 's':  // sort
        {
            if (supported_flags & COMMON_SORT)
            {
                opts->sort = (opt_arg && opt_arg[0] != '\0') ? opt_arg : "name";
            }
            break;
        }
        case 'R':  // recursive
        {
            if (supported_flags & COMMON_RECURSIVE)
            {
                opts->recursive = true;
            }
            break;
        }
        case 'A': // almost-all
        {
            if (supported_flags & COMMON_ALMOST_ALL)
            {
                opts->almost_all = true;
                opts->all = false;
            }
            break;
        }
    }
}

void handle_filter_flag(int opt, const char *opt_name, char *opt_arg,
                         FilterOptions *opts, uint32_t supported_flags)
{
    switch (opt)
    {
        case 'c':  // contains
        {
            if (supported_flags & FILTER_CONTAINS)
            {
                opts->contains = opt_arg;
            }
            break;
        }
        case 'e':  // extension
        {
            if (supported_flags & FILTER_EXTENSION)
            {
                opts->extension = opt_arg;
            }
            break;
        }
        case 't':  // type
        {
            if (supported_flags & FILTER_TYPE)
            {
                opts->type = opt_arg;
            }
            break;
        }
        case 0:  // max-size | min-size
        {
            if (strcmp(opt_name, "max-size") == 0)
            {
                if (supported_flags & FILTER_MAX_SIZE)
                {
                    opts->max_size = get_size(opt_arg);
                }
            }
            else if (strcmp(opt_name, "min-size") == 0)
            {
                if (supported_flags & FILTER_MIN_SIZE)
                {
                    opts->min_size = get_size(opt_arg);
                }
            }
            break;
        }
    }
}

void handle_action_flag(int opt, ActionOptions *opts, uint32_t supported_flags)
{
    switch (opt)
    {
        case 'd':  // dry-run
        {
            if (supported_flags & ACTION_DRY_RUN)
            {
                opts->dry_run = true;
            }
            break;
        }
        case 'i':  // interactive
        {
            if (supported_flags & ACTION_INTERACTIVE)
            {
                opts->interactive = true;
            }
            break;
        }
        case 'v':  // verbose
        {
            if (supported_flags & ACTION_VERBOSE)
            {
                opts->verbose = true;
            }
            break;
        }
    }
}
