#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>  // intmax_t
#include <limits.h>  // PATH_MAX
#include <stdbool.h>
#include <stdint.h>  // uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "cli_parse.h"
#include "commands.h"
#include "delete.h"
#include "help.h"
#include "log.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void delete_element(const struct dirent *namelist, DeleteOptions *opts,
                           Extension *ext, const char *base_dir, const char *current_path,
                           DeleteCounters *counters, ScandirFilter filter, bool nuke_del);
static bool delete_directory(DeleteOptions *opts, const char *path,
                             DeleteCounters *counters, ScandirFilter filter);
static void update_log_file_delete(const char *src_path);

// Setup logic for 'delete' feature
ErrorCode handle_delete(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_delete_help,
                                            parse_delete_opts, sizeof(DeleteOptions));
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

    DeleteOptions *opts = (DeleteOptions*)context->opts;
    // Initializes counters
    DeleteCounters counters = {0};

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

    // Parses directory's content
    for (int i = 0; i < n; i++)
    {
        delete_element(namelist[i], opts, ext, context->base_dir,
                       context->base_dir, &counters, context->filter, false);
    }
    free_dirent(namelist, n);

    // Output message
    char *f_out = formatted_output(counters.dlt_size);
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Files deleted: %zu\n"
               "[DRY-RUN] Directories deleted: %zu\n"
               "[DRY-RUN] Space freed: %s\n",
               counters.dlt_files, counters.dlt_directories, f_out);
    }
    else
    {
        printf("Deleted files: %zu\n"
               "Deleted directories: %zu\n",
               counters.dlt_files, counters.dlt_directories);

        if (opts->base.human_readable)
        {
            printf("Space freed: %s\n", f_out);
        }
        else
        {
            printf("Space freed: %jd bytes\n", (intmax_t)counters.dlt_size);
        }
    }

    free(f_out);

    print_counter_err_msg(counters.error);

cleanup:
    if (ext)
    {
        free_extensions(ext, counters.ext);
    }
    
    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'delete' functionality
ErrorCode parse_delete_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    DeleteOptions *opts = (DeleteOptions*)opts_out;

    opterr = 0;

    // Supported flags
    uint32_t common_flags = COMMON_ALL |
                            COMMON_ALMOST_ALL |
                            COMMON_HUMAN_READABLE |
                            COMMON_RECURSIVE;
    uint32_t filter_flags = FILTER_CONTAINS |
                            FILTER_EXTENSION |
                            FILTER_MAX_SIZE |
                            FILTER_MIN_SIZE |
                            FILTER_TYPE;
    uint32_t action_flags = ACTION_DRY_RUN |
                            ACTION_INTERACTIVE |
                            ACTION_VERBOSE;

    static struct option long_opts[] =
    {
        // Common flags
        {"all", no_argument, 0, 'a'},
        {"almost-all", no_argument, 0, 'A'},
        {"human-readable", no_argument, 0, 'h'},
        {"recursive", no_argument, 0, 'R'},
        // Filter flags
        {"contains", required_argument, 0, 'c'},
        {"extension", required_argument, 0, 'e'},
        {"max-size", required_argument, 0, 0},
        {"min-size", required_argument, 0, 0},
        {"type", required_argument, 0, 't'},
        // Action flags
        {"dry-run", no_argument, 0, 'd'},
        {"interactive", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "aAhRc:e:t:div";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // ========== COMMON ==========
            case 'a':  // all
            case 'A':  // almost-all
            case 'h':  // human-readable
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
            // ========== ACTION ==========
            case 'd':  // dry-run
            case 'i':  // interactive
            case 'v':  // verbose
            {
                handle_action_flag(opt, &opts->action, action_flags);
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

// Deletes elements
static void delete_element(const struct dirent *namelist, DeleteOptions *opts,
                           Extension *ext, const char *base_dir, const char *current_path,
                           DeleteCounters *counters, ScandirFilter filter, bool nuke_del)
{
    // Creates full path to current element
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        counters->error++;
        return;
    }

    struct stat st;
    bool has_data = false;

    // Checks for directory type
    bool is_dir = (namelist->d_type == DT_DIR);
    if (namelist->d_type == DT_UNKNOWN)
    {
        if (lstat(full_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
            counters->error++;
            return;
        }

        is_dir = S_ISDIR(st.st_mode);
        has_data = true;
    }

    // Gets source's suffix (cleaner output)
    const char *src_suffix = get_suffix(full_path, base_dir);

    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        // Checks directory's validity
        if (check_directory_flags(namelist, full_path, &opts->filter, filter))
        {
            if (!nuke_del && opts->action.interactive && !opts->action.dry_run)
            {
                char *prompt = NULL;
                if (asprintf(&prompt, "Fully remove directory '%s'?", src_suffix) == -1)
                {
                    fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                    counters->error++;
                    if (prompt)
                    {
                        free(prompt);
                    }
                    return;
                }
                if (!get_answer(prompt))
                {
                    free(prompt);
                    return;
                }

                nuke_del = true;
                free(prompt);
            }
            if (opts->action.dry_run)
            {
                printf("[DRY-RUN] Would fully remove directory %s\n", src_suffix);
                counters->dlt_directories++;
                nuke_del = true;
                return;
            }
            else
            {
                if (delete_directory(opts, full_path, counters, filter))
                {
                    if (opts->action.verbose)
                    {
                        printf("Fully deleted directory: '%s'\n", src_suffix);
                    }
                    counters->dlt_directories++;
                    log_write(LOG_SUCCESS, CMD_DELETE, full_path);
                }
                else
                {
                    counters->error++;
                    update_log_file_delete(full_path);
                }
            }

            return;
        }

        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(full_path, &entry, filter, alphasort);
            if (n == -1)
            {
                fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
                counters->error++;
                return;
            }

            for (int i = 0; i < n; i++)
            {
                delete_element(entry[i], opts, ext, base_dir,
                               full_path, counters, filter, nuke_del);
            }
            free_dirent(entry, n);

            // Tries to remove current directory (if empty after recursion)
            if (!opts->action.dry_run)
            {
                if (rmdir(full_path) == 0)
                {
                    counters->dlt_directories++;
                    log_write(LOG_SUCCESS, CMD_DELETE, full_path);
                }
                else
                {
                    counters->error++;
                    update_log_file_delete(full_path);
                }
            }
        }

        return;
    }

    // ================== FILES & SLINKS ===================
    if (!nuke_del)
    {
        // Checks file's validity
        if (!check_file_flags(namelist, ext, full_path, &opts->filter,
                              counters->ext, &counters->error))
        {
            return;
        }
        
        if (opts->action.interactive && !opts->action.dry_run)
        {
            char *prompt = NULL;
            if (asprintf(&prompt, "Delete file: '%s'?", src_suffix) == -1)
            {
                fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                counters->error++;
                if (prompt)
                {
                    free(prompt);
                }
                return;
            }
            if (!get_answer(prompt))
            {
                free(prompt);
                return;
            }

            free(prompt);
        }
    }

    if (!has_data)
    {
        if (lstat(full_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
            counters->error++;
            return;
        }
    }

    if (opts->action.dry_run)
    {
        if (opts->action.verbose)
        {
            printf("[DRY-RUN] Would delete file: '%s'\n", src_suffix);
        }
        counters->dlt_files++;
        counters->dlt_size += st.st_size;
    }
    else
    {
        // Deletes file
        if (remove(full_path) == 0)
        {
            if (opts->action.verbose)
            {
                printf("File deleted: '%s'\n", src_suffix);
            }

            counters->dlt_files++;
            counters->dlt_size += st.st_size;
            log_write(LOG_SUCCESS, CMD_DELETE, full_path);
        }
        else
        {
            counters->error++;
            update_log_file_delete(full_path);
        }
    }
}

// Fully deletes a directory
static bool delete_directory(DeleteOptions *opts, const char *path,
                             DeleteCounters *counters, ScandirFilter filter)
{
    struct dirent **ptr = NULL;
    int n = scandir(path, &ptr, filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        counters->error++;
        return false;
    }

    for (int i = 0; i < n; i ++)
    {
        char new_path[PATH_MAX];
        if (check_path_name_size(new_path, sizeof(new_path), path, ptr[i]->d_name) == -1)
        {
            fprintf(stderr, "Path too long: %s/%s\n", new_path, ptr[i]->d_name);
            counters->error++;
            continue;
        }
    
        bool is_dir = (ptr[i]->d_type == DT_DIR);
        if (ptr[i]->d_type == DT_UNKNOWN)
        {
            struct stat st;
            if (lstat(new_path, &st) != 0)
            {
                fprintf(stderr, "Couldn't access %s: %s\n", new_path, strerror(errno));
                counters->error++;
                continue;
            }

            is_dir = S_ISDIR(st.st_mode);
        }

        // ==================== DIRECTORIES ====================
        if (is_dir)
        {
            // Recursive call for deletion
            delete_directory(opts, new_path, counters, filter);
        }

        // ================== FILES & SLINKS ===================
        else
        {
            struct stat st;
            if (lstat(new_path, &st) == 0)
            {
                if (remove(new_path) == 0)
                {
                    counters->dlt_files++;
                    counters->dlt_size += st.st_size;
                    log_write(LOG_SUCCESS, CMD_DELETE, new_path);
                }
                else
                {
                    counters->error++;
                    update_log_file_delete(new_path);
                }
            }
            else
            {
                fprintf(stderr, "Couldn't access %s: %s\n", new_path, strerror(errno));
                counters->error++;
                continue;
            }
        }
    }
    free_dirent(ptr, n);

    // Removes current directory
    return (rmdir(path) == 0);
}

// Updates log file for delete feature
static void update_log_file_delete(const char *src_path)
{
    char log_msg[512] = {0};
    snprintf(log_msg, sizeof(log_msg), "%s (%s)", src_path, strerror(errno));
    log_write(LOG_ERROR, CMD_DELETE, log_msg);
}
