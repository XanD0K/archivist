// Libraries
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>  // NULL
#include <string.h>
#include <unistd.h>

// Headers
#include "cli_parse_common.h"
#include "utils.h"

int parse_common_opts(int argc, char **argv, int opt_start, CommonOptions *opts)
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
                opts->human_readable = true;
                break;
            }
            case 'i':
            {
                opts->ignore_case = false;
                break;
            }
            case 's':
            {
                opts->sort = optarg;
                break;
            }
            case 'R':
            {
                opts->recursive = true;
                break;
            }
            case '?':
            {
                return 1;
            }
        }
    }

    return 0;
}

int parse_filter_options(int argc, char **argv, int opt_start, FilterOptions *opts)
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
                opts->contains = optarg;
                break;
            }
            case 'e':
            {
                opts->extension = optarg;
                break;
            }
            case 't':
            {
                opts->type = optarg;
                break;
            }
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "max-size") == 0)
                {
                    opts->max_size = get_size(optarg);
                }
                else if (strcmp(long_opts[long_index].name, "min-size") == 0)
                {
                    opts->min_size = get_size(optarg);
                }
                break;
            }
            case '?':
            {
                return 1;
            }
        }
    }

    if (opts->max_size == -1 || opts->min_size == -1)
    {
        return PARSE_ERROR_INVALID_SIZE;
    }

    return 0;
}

int parse_action_options(int argc, char **argv, int opt_start, ActionOptions *opts)
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
                opts->dry_run = true;
                break;
            }
            case 'i':
            {
                opts->interactive = true;
                break;
            }
            case 'v':
            {
                opts->verbose = true;
                break;
            }
            case 0:
            {
                break;
            }

            case '?':
            {
                return 1;
            }
        }
    }

    return 0;
}
