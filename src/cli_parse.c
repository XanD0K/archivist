// Libraries
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>  // NULL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>  // optind | optarg

// Headers
#include "cli_parse.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static off_t get_size(const char *size);

// Checks common flags
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
        case 'A': // almost-all
        {
            if (supported_flags & COMMON_ALMOST_ALL)
            {
                opts->almost_all = true;
                opts->all = false;
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
    }
}

// Checks filter flags
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

// Checks action flags
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

// Converts user's input to off_t size
static off_t get_size(const char *size)
{
    const off_t BUFFER = 1024;
    char *ptr;
    errno = 0;
    long num = strtol(size, &ptr, 10);

    if (errno == EINVAL || errno == ERANGE)
    {
        fprintf(stderr, "Error on strtol(): %s\n", strerror(errno));
        return -1;
    }
    if (num < 0)
    {
        fprintf(stderr, "Size can't be negative: %ld\n", num);
        return -1;
    }
    if (ptr == size)
    {
        fprintf(stderr, "No digits were found!\n");
        return -1;
    }

    num = (off_t)num;

    if(ptr == NULL || *ptr == '\0' || strcasecmp(ptr, "B") == 0)
    {
        return num;
    }
    else if (strcasecmp(ptr, "K") == 0 || strcasecmp(ptr, "KB") == 0)
    {
        return num * BUFFER;
    }
    else if (strcasecmp(ptr, "M") == 0 || strcasecmp(ptr, "MB") == 0)
    {
        return num * BUFFER * BUFFER;
    }
    else if (strcasecmp(ptr, "G") == 0 || strcasecmp(ptr, "GB") == 0)
    {
        return num * BUFFER * BUFFER * BUFFER;
    }
    else if (strcasecmp(ptr, "T") == 0 || strcasecmp(ptr, "TB") == 0)
    {
        return num * BUFFER * BUFFER * BUFFER * BUFFER;
    }

    return -1;
}
