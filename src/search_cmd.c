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
static void search_element(const struct dirent *namelist, const char *base_dir,
                           const char *current_path, SearchOptions *opts, SearchCounters *counters);

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
        return (context->error_code == EC_HELP_FLAG || context->error_code == EC_CMD_HELP_FLAG)
            ? EC_SUCCESS
            : context->error_code;
    }

    SearchOptions *opts = (SearchOptions*)context->opts;

    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, scandir_no_dot_filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        free_command_context(context);
        return EC_SCANDIR_ERROR;
    }

    // Initializes counters
    SearchCounters counters = {0};
    for (int i = 0; i < n; i++)
    {
        search_element(namelist[i], context->base_dir, context->base_dir, opts, &counters);
    }
    free_dirent(namelist, n);

    if (counters.searched == 0)
    {
        printf("No elements found!\n");
    }
    else
    {
        printf("Total elements found: %zu\n", counters.searched);
    }

    if (counters.error != 0)
    {
        printf("(Finished with %zu erros)\n", counters.error);
    }

    free_command_context(context);
    return EC_SUCCESS;
}

// Parses through CLI arguments for 'search' functionality
ErrorCode parse_search_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    SearchOptions *opts = (SearchOptions*)opts_out;

    uint32_t supported_flags = COMMON_IGNORE_CASE |
                               COMMON_RECURSIVE |
                               FILTER_CONTAINS |
                               FILTER_EXTENSION |
                               FILTER_MAX_SIZE |
                               FILTER_MIN_SIZE |
                               FILTER_TYPE;

    ErrorCode ret;
    ret = parse_common_opts(argc, argv, opt_start, &opts->base, supported_flags);
    if (ret != EC_SUCCESS)
    {
        return ret;
    }
    ret = parse_filter_options(argc, argv, opt_start, &opts->filter, supported_flags);
    if (ret != EC_SUCCESS)
    {
        if (ret == EC_PARSE_ERROR_SIZE)
        {
            errno = EIO;
            fprintf(stderr, "Invalid size: %s\n", strerror(errno));
        }
        return ret;
    }

    return EC_SUCCESS;
}

// Searches for an element
static void search_element(const struct dirent *namelist, const char *base_dir,
                           const char *current_path, SearchOptions *opts, SearchCounters *counters)
{
    // Builds path to current element
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        return;
    }

    // Checks for directory type
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

    // ==================== Directories ====================
    if (is_dir)
    {
        bool should_print = true;
        // Checks directory's filters
        if (opts->filter.type && opts->filter.type[0] != '\0')
        {
            if (!is_directory_type(opts->filter.type))
            {
                should_print = false;
            }
           
        }

        if (should_print && opts->filter.contains && opts->filter.contains[0] != '\0')
        {
            if (!match_searched_name(namelist->d_name, opts->filter.contains, opts->base.ignore_case))
            {
                should_print = false;
            }
        }

        if (should_print && (opts->filter.max_size || opts->filter.min_size))
        {
            off_t dir_size = 0;
            if (!match_directory_size(full_path, opts->filter.max_size, opts->filter.min_size, &dir_size))
            {
                should_print = false;
            }
        }

        if (should_print)
        {
            // Prints found element
            const char *suffix = get_suffix(full_path, base_dir);
            printf("%s\n", suffix);
            counters->searched++;
        }

        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            struct dirent **entry = NULL;
            int n = scandir(full_path, &entry, scandir_no_dot_filter, alphasort);
            if (n == -1)
            {
                fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
                counters->error++;
                return;
            }
            for (int i = 0; i < n; i++)
            {
                search_element(entry[i], base_dir, full_path, opts, counters);
            }
            free_dirent(entry, n);
        }

        return;
    }
    // ======================= Files =======================
    else if (is_file || is_slink)
    {
        bool should_print = true;

        // Name filter
        if (opts->filter.contains && opts->filter.contains[0] != '\0')
        {
            if (!match_searched_name(namelist->d_name, opts->filter.contains, opts->base.ignore_case))
            {
                should_print = false;
            }
        }

        // Extension filter
        if (should_print && is_file && opts->filter.extension &&
            !match_searched_extension(opts->filter.extension, namelist->d_name))
        {
            should_print = false;
        }

        if (should_print &&
            (opts->filter.type || opts->filter.max_size || opts->filter.min_size))
        {
            struct stat st;
            if (lstat(full_path, &st) != 0)
            {
                fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
                counters->error++;
                should_print = false;
            }

            // Checks for element's type
            if (is_file && opts->filter.type && opts->filter.type[0] != '\0')
            {
                if (!match_type(opts->filter.type, st.st_mode))
                {
                    should_print = false;
                }
            }

            // Checks for size
            if (should_print && (opts->filter.max_size || opts->filter.min_size) &&
                !match_size(opts->filter.max_size, opts->filter.min_size, st.st_size))
            {
                should_print = false;
            }
        }

        if (should_print)
        {
            // Prints found element
            const char *suffix = get_suffix(full_path, base_dir);
            printf("%s\n", suffix);
            counters->searched++;
        }
    }

    return;
}
