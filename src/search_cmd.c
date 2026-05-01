#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>  // PATH_MAX
#include <stdint.h>  // uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>  // off_t
#include <unistd.h>

// Headers
#include "cli_parse.h"
#include "commands.h"
#include "help.h"
#include "search_cmd.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void search_element(const struct dirent *namelist, SearchOptions *opts,
                           Extension *ext, const char *base_dir, const char *current_path,
                           SearchCounters *counters, ScandirFilter filter);

// Setup logic for 'search' feature
ErrorCode handle_search(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_search_help,
                                            parse_search_opts, sizeof(SearchOptions));
    if (!context)
    {
        return EC_MEMORY_ALLOCATION;
    }
    if (context->error_code != EC_SUCCESS)
    {
        free_command_context(context);
        return (context->error_code == EC_CMD_HELP_FLAG)
            ? EC_SUCCESS
            : context->error_code;
    }

    SearchOptions *opts = (SearchOptions*)context->opts;

    // Initializes counters
    SearchCounters counters = {0};

    // Retrieves user's selected extensions (-e|--extension)
    Extension *ext = NULL;
    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        ext = get_all_extensions(opts->filter.extension, &counters.ext);
        if (!ext)
        {
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            free_command_context(context);
            return EC_MEMORY_ALLOCATION;
        }
    }

    // Default return value
    ErrorCode ret = EC_SUCCESS;

    // Changes default filter used in scandir()
    if (opts->base.all)
    {
        context->filter = scandir_no_dot_filter;
    }
    else if (opts->base.almost_all)
    {
        context->filter = scandir_show_hidden_files;
    }

    // Retrieves directory's content
    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, context->filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        ret = EC_SCANDIR_ERROR;
        goto cleanup;
    }

    for (int i = 0; i < n; i++)
    {
        search_element(namelist[i], opts, ext, context->base_dir,
                       context->base_dir, &counters, context->filter);
    }
    free_dirent(namelist, n);

    // Prints output message
    if (counters.searched == 0)
    {
        printf("No elements found!\n");
    }
    else
    {
        print_divider();
        printf("Total elements found: %zu\n", counters.searched);
    }
    print_counter_err_msg(counters.error);

// Cleans allocated memory
cleanup:
    if (ext)
    {
        free_extensions(ext, counters.ext);
    }
    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'search' functionality
ErrorCode parse_search_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    SearchOptions *opts = (SearchOptions*)opts_out;

    opterr = 0;

    // Supported flags
    uint32_t common_flags = COMMON_ALL |
                            COMMON_ALMOST_ALL |
                            COMMON_RECURSIVE;
    uint32_t filter_flags = FILTER_CONTAINS |
                            FILTER_EXTENSION |
                            FILTER_MAX_SIZE |
                            FILTER_MIN_SIZE |
                            FILTER_TYPE;

    static struct option long_opts[] =
    {
        // Common flags
        {"all", no_argument, 0, 'a'},
        {"almost-all", no_argument, 0, 'A'},
        {"recursive", no_argument, 0, 'R'},
        // Filter flags
        {"contains", required_argument, 0, 'c'},
        {"extension", required_argument, 0, 'e'},
        {"max-size", required_argument, 0, 0},
        {"min-size", required_argument, 0, 0},
        {"type", required_argument, 0, 't'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "aARc:e:t:";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // ========== COMMON ==========
            case 'a':  // all
            case 'A':  // almost-all
            case 'R':  // recursive
            {
                handle_common_flag(opt, optarg, &opts->base, common_flags);
                break;
            }
            // ========== FILTER ==========
            case 'c':  // contains
            case 'e':  // extension
            case 't':  // type
            case 0:    // max-size | min-size 
            {
                handle_filter_flag(opt, long_opts[long_index].name, optarg,
                                   &opts->filter, filter_flags);
                break;
            }
            // ========== ERROR ===========
            case '?':
            {
                fprintf(stderr, "Flag not allowed: %s\n", argv[optind - 1]);
                return EC_PARSE_ERROR;
            }
        }
    }

    // Checks for invalid argument
    if (optind < argc)
    {
        fprintf(stderr, "Invalid argument(s):");
        while (optind < argc)
        {
            fprintf(stderr, " %s", argv[optind++]);
        }
        fprintf(stderr, "\n");
        return EC_PARSE_ERROR;
    }

    // Defines filter
    if (opts->base.almost_all)
    {
        opts->base.all = false;
    }
   
    // Validates size
    if (opts->filter.max_size == -1 || opts->filter.min_size == -1)
    {
        return EC_PARSE_ERROR_SIZE;
    }

    // Validates type
    if (opts->filter.type && opts->filter.type[0] != '\0')
    {
        if (!validate_type(opts->filter.type))
        {
            return EC_PARSE_ERROR_TYPE;
        }
    }

    opts->filter.supported = filter_flags;
    return EC_SUCCESS;
}

// Searches for an element
static void search_element(const struct dirent *namelist, SearchOptions *opts,
                           Extension *ext, const char *base_dir, const char *current_path,
                           SearchCounters *counters, ScandirFilter filter)
{
    // Builds path to current element
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        counters->error++;
        return;
    }

    // Gets path's suffix
    const char *suffix = get_suffix(full_path, base_dir);

    // Checks for content type
    bool is_dir = (namelist->d_type == DT_DIR);
    bool is_file = (namelist->d_type == DT_REG);
    bool is_slink = (namelist->d_type == DT_LNK);
    if (namelist->d_type == DT_UNKNOWN || (!is_dir && !is_file && !is_slink))
    {
        struct stat st;
        if (lstat(full_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
            counters->error++;
            return;
        }

        is_dir = S_ISDIR(st.st_mode);
        is_file = S_ISREG(st.st_mode);
        is_slink = S_ISLNK(st.st_mode);
    }

    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        // Checks directory's validity
        if (check_directory_flags(namelist, full_path, &opts->filter, filter))
        {
            // Prints found element
            printf("%s/\n", suffix);
            counters->searched++;
        }

        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            struct dirent **entry = NULL;
            int n = scandir(full_path, &entry, filter, alphasort);
            if (n == -1)
            {
                fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
                counters->error++;
                return;
            }
            for (int i = 0; i < n; i++)
            {
                search_element(entry[i], opts, ext, base_dir,
                               full_path, counters, filter);
            }
            free_dirent(entry, n);
        }

        return;
    }
    // ======================= FILES =======================
    else if (is_file || is_slink)
    {
        // Checks file's validity
        if (check_file_flags(namelist, ext, full_path, &opts->filter,
                             counters->ext, &counters->error))
        {
            // Prints found element
            printf("%s\n", suffix);
            counters->searched++;
        }
    }
}
