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

ErrorCode parse_common_opts(int argc, char **argv, int opt_start, CommonOptions *opts, uint32_t supported_flags)
{
    static struct option long_opts[] = 
    {
        {"human-readable", no_argument, 0, 'h'},
        {"ignore-case", no_argument, 0, 'i'},
        {"sort", required_argument, 0, 's'},
        {"recursive", no_argument, 0, 'R'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0, long_index = 0;
    char *short_opts = "his:R";

    optind = opt_start;

    while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'h':
            {
                if (!(supported_flags & COMMON_HUMAN_READABLE))
                {
                    goto unsupported;
                }
                opts->human_readable = true;
                break;
            }
            case 'i':
            {
                if (!(supported_flags & COMMON_IGNORE_CASE ))
                {
                    goto unsupported;
                }
                opts->ignore_case = true;
                break;
            }
            case 's':
            {
                if (!(supported_flags & COMMON_SORT))
                {
                    goto unsupported;
                }
                opts->sort = optarg;
                break;
            }
            case 'R':
            {
                if (!(supported_flags & COMMON_RECURSIVE))
                {
                    goto unsupported;
                }
                opts->recursive = true;
                break;
            }
            case '?':
            {
                fprintf(stderr, "Invalid flag: %s\n", argv[optind - 1]);
                return EC_PARSE_ERROR;
            }
        }
    }

    return EC_SUCCESS;

unsupported:
    fprintf(stderr, "Unsupported flag: %s\n", argv[optind - 1]);
    return EC_PARSE_ERROR_UNSUPPORTED;
}

ErrorCode parse_filter_options(int argc, char **argv, int opt_start, FilterOptions *opts, uint32_t supported_flags)
{
    static struct option long_opts[] = 
    {
        {"contains", required_argument, 0, 'c'},
        {"extension", required_argument, 0, 'e'},
        {"type", required_argument, 0, 't'},
        {"max-size", required_argument, 0, 0},
        {"min-size", required_argument, 0, 0},
        {NULL, 0, NULL, 0}
    };

    int opt = 0, long_index = 0;
    char *short_opts = "c:e:t:";

    optind = opt_start;

    while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'c':
            {
                if (!(supported_flags & FILTER_CONTAINS))
                {
                    goto unsupported;
                }
                opts->contains = optarg;
                break;
            }
            case 'e':
            {
                if (!(supported_flags & FILTER_EXTENSION))
                {
                    goto unsupported;
                }
                opts->extension = optarg;
                break;
            }
            case 't':
            {
                if (!(supported_flags & FILTER_TYPE))
                {
                    goto unsupported;
                }
                opts->type = optarg;
                break;
            }
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "max-size") == 0)
                {
                    if (!(supported_flags & FILTER_MAX_SIZE))
                    {
                        goto unsupported;
                    }
                    opts->max_size = get_size(optarg);
                }
                else if (strcmp(long_opts[long_index].name, "min-size") == 0)
                {
                    if (!(supported_flags & FILTER_MIN_SIZE))
                    {
                        goto unsupported;
                    }
                    opts->min_size = get_size(optarg);
                }
                break;
            }
            case '?':
            {
                fprintf(stderr, "Invalid flag: %s\n", argv[optind - 1]);
                return EC_PARSE_ERROR;
            }
        }
    }

    if (opts->max_size == -1 || opts->min_size == -1)
    {
        return EC_PARSE_ERROR_SIZE;
    }

    return EC_SUCCESS;

unsupported:
    fprintf(stderr, "Unsupported flag: %s\n", argv[optind - 1]);
    return EC_PARSE_ERROR_UNSUPPORTED;
}

ErrorCode parse_action_options(int argc, char **argv, int opt_start, ActionOptions *opts, uint32_t supported_flags)
{
    static struct option long_opts[] = 
    {
        {"dry-run", no_argument, 0, 'd'},
        {"interactive", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0, long_index = 0;
    char *short_opts = "div";

    optind = opt_start;

    while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'd':
            {
                if (!(supported_flags & ACTION_DRY_RUN))
                {
                    goto unsupported;
                }
                opts->dry_run = true;
                break;
            }
            case 'i':
            {
                if (!(supported_flags & ACTION_INTERACTIVE))
                {
                    goto unsupported;
                }
                opts->interactive = true;
                break;
            }
            case 'v':
            {
                if (!(supported_flags & ACTION_VERBOSE))
                {
                    goto unsupported;
                }
                opts->verbose = true;
                break;
            }
            case '?':
            {
                fprintf(stderr, "Invalid flag: %s\n", argv[optind - 1]);
                return EC_PARSE_ERROR;
            }
        }
    }

    return EC_SUCCESS;

unsupported:
    fprintf(stderr, "Unsupported flag: %s\n", argv[optind - 1]);
    return EC_PARSE_ERROR_UNSUPPORTED;
}
