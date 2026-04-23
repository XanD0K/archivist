#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <errno.h>
#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "cli_parse.h"
#include "commands.h"
#include "help.h"
#include "log.h"
#include "rename.h"
#include "utils.h"
#include "utils_filter.h"
#include "utils_sort.h"

// Prototypes
static void rename_element(struct dirent *namelist, Extension *ext, const char *base_dir,
                           const char *current_path, SortFlag sorter, RenameOptions *opts,
                           RenameCounters *counters);
static char *generate_unique_name(const char *current_path, char *old_name,
                                  char *input_name, size_t *counter);

// Setup logic for 'rename' feature
ErrorCode handle_rename(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_rename_help,
                                            parse_rename_options, sizeof(RenameOptions));
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

    RenameOptions *opts = (RenameOptions*)context->opts;
    cmp_opts.base_dir = context->base_dir;

    // Default sorter function (by name)
    SortFlag sorter = cmp_name_scandir;
    if (opts->base.sort && opts->base.sort[0] != '\0' && 
        strcasecmp(opts->base.sort, "name") != 0)
    {
        // All sort methods available
        const char *sorts[] = {"date", "size", "version"};
        size_t len = sizeof(sorts) / sizeof(sorts[0]);

        // Updates sorter function
        sorter = get_sort_function(opts->base.sort, sorts, len);
        if (!sorter)
        {
            free_command_context(context);
            return EC_INVALID_SORTER;
        }
    }

    RenameCounters counters = {0};
    counters.name = 1;

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

    // Gets directory's content
    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, scandir_no_dot_filter, sorter);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        ret = EC_SCANDIR_ERROR;
        goto cleanup;
    }

    for (int i = 0; i < n; i++)
    {
        rename_element(namelist[i], ext, context->base_dir, context->base_dir, sorter, opts, &counters);
    }
    free_dirent(namelist, n);

    // Output Message
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Would rename %zu files\n", counters.rnmd_files);
    }
    else
    {
        printf("Renamed files: %zu\n", counters.rnmd_files);
    }

    if (counters.error != 0)
    {
        printf("(Finished with %zu errors)\n", counters.error);
    }

cleanup:
    free_extensions(ext, counters.ext);
    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'rename' functionality
ErrorCode parse_rename_options(int argc, char **argv, int opt_start, void *opts_out)
{
    RenameOptions *opts = (RenameOptions*)opts_out;
    opts->base.sort = "name";

    uint32_t supported_flags = COMMON_RECURSIVE |
                               COMMON_SORT |
                               FILTER_CONTAINS |
                               FILTER_EXTENSION |
                               FILTER_MAX_SIZE |
                               FILTER_MIN_SIZE |
                               ACTION_DRY_RUN |
                               ACTION_INTERACTIVE |
                               ACTION_VERBOSE;

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

    ret = parse_action_options(argc, argv, opt_start, &opts->action, supported_flags);
    if (ret != EC_SUCCESS)
    {
        return ret;
    }   

    static struct option long_opts[] =
    {
        {"name", required_argument, 0, 'n'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0,  long_index = 0;
    char *short_opts = "n:";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'n':
            {
                opts->name = optarg;
                break;
            }
            // Error
            case '?':
            {
                fprintf(stderr, "Flag not allowed: %s\n", argv[optind - 1]);
                return EC_PARSE_ERROR;
            }
        }
    }

    return EC_SUCCESS;
}

// Renames files from given directory
static void rename_element(struct dirent *namelist, Extension *ext, const char *base_dir,
                           const char *current_path, SortFlag sorter, RenameOptions *opts,
                           RenameCounters *counters)
{
    char old_path[PATH_MAX];
    if (check_path_name_size(old_path, sizeof(old_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        return;
    }
    
    bool is_dir = (namelist->d_type == DT_DIR);
    if (!is_dir && namelist->d_type == DT_UNKNOWN)
    {
        struct stat st;
        if (lstat(old_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", old_path, strerror(errno));
            counters->error++;
            return;
        }

        is_dir = S_ISDIR(st.st_mode);
    }

    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(old_path, &entry, scandir_no_dot_filter, sorter);
            if (n == -1)
            {
                return;
            }

            counters->name = 1;
            for (int i = 0; i < n; i++)
            {
                rename_element(entry[i], ext, base_dir, old_path, sorter, opts, counters);
            }
            free_dirent(entry, n);
        }
    }
    // ================== FILES & SLINKS ===================
    else
    {
        if (opts->filter.contains && opts->filter.contains[0] != '\0' &&
            !match_name(opts->filter.contains, namelist->d_name))
        {
            return;
        }

        // Checks for matching extension
        if (ext && !match_extension(ext, counters->ext, namelist->d_name))
        {
            return;
        }

        // Checks for size
        if (opts->filter.max_size || opts->filter.min_size)
        {
            struct stat st;
            if (lstat(old_path, &st) != 0)
            {
                fprintf(stderr, "Couldn't access %s: %s\n", old_path, strerror(errno));
                counters->error++;
                return;
            }

            if (!match_size(opts->filter.max_size, opts->filter.min_size, st.st_size))
            {
                return;
            }
        }

        // Gets new name
        char *new_path = generate_unique_name(current_path, namelist->d_name, opts->name, &counters->name);
        if (!new_path)
        {
            return;
        }

        if (opts->action.interactive)
        {
            char *prompt = NULL;
            if (asprintf(&prompt, "Rename file %s to %s? ", old_path, new_path) == -1)
            {
                fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                free(new_path);
                free(prompt);
                return;
            }
            if (!get_answer(prompt))
            {
                free(new_path);
                free(prompt);
                return;
            }

            free(prompt);
        }

        if (opts->action.dry_run)
        {
            printf("[DRY-RUN] Would rename file '%s' to '%s'\n", old_path, new_path);
            counters->rnmd_files++;
        }
        else
        {
            // Renames files
            if (rename(old_path, new_path) == 0)
            {
                if (opts->action.verbose)
                {
                    printf("File '%s' renamed to '%s'\n", namelist->d_name, new_path);
                }

                update_log_file(LOG_SUCCESS, CMD_RENAME, namelist->d_name, new_path, false);
                counters->rnmd_files++;
            }
            else
            {
                update_log_file(LOG_ERROR, CMD_RENAME, namelist->d_name, new_path, true);
                counters->error++;
            }
        }

        free(new_path);
    }
}

// Defines behaviour when file already exists on destination
static char *generate_unique_name(const char *current_path, char *old_name,
                                  char *input_name, size_t *counter)
{
    // Default behavior: incremental rename
    const char *dot = strrchr(old_name, '.');
    const char *ext = (dot) ? dot : "";

    char new_name[PATH_MAX];
    char full_path[PATH_MAX];

    while (1)
    {
        snprintf(new_name, sizeof(new_name), "%s_%zu%s", input_name, (*counter), ext);

        if (check_path_name_size(full_path, sizeof(full_path), current_path, new_name) == -1)
        {
            return NULL;
        }

        if (access(full_path, F_OK) != 0)
        {
            return strdup(full_path);
        }

        (*counter)++;
    }
}
