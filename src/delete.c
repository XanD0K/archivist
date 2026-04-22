#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
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
static void delete_element(const char *current_path, DeleteOptions *opts, struct dirent *namelist,
                           Extension *ext, DeleteCounters *counters);
static bool delete_directory(DeleteOptions *opts, const char *path, DeleteCounters *counters);
static void update_log_file_delete(const char *src_path);

// Setup logic for 'delete' feature
ErrorCode handle_delete(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_delete_help,
                                            parse_delete_options, sizeof(DeleteOptions));
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

    DeleteOptions *opts = (DeleteOptions*)context->opts;
    // Initializes counters
    DeleteCounters counters = {0};

    // Retrieves user's typed extensions
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

    // Retrieves directory's content
    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, scandir_no_dot_filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        ret = EC_SCANDIR_ERROR;
        goto cleanup;
    }

    // Parses directory's content
    for (int i = 0; i < n; i++)
    {
        delete_element(context->base_dir, opts, namelist[i], ext, &counters);
    }

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
        printf("Files deleted: %zu\n"
               "Directories deleted: %zu\n",
               counters.dlt_files, counters.dlt_directories);

        if (opts->base.human_readable)
        {
            printf("Space freed: %s\n", f_out);
        }
        else
        {
            printf("Space freed: %jd\n", (intmax_t)counters.dlt_size);
        }
    }

    free(f_out);

    if (counters.error != 0)
    {
        printf("(Finished with %zu errors)\n", counters.error);
    }

cleanup:
    if (namelist)
    {
        free_dirent(namelist, n);
    }
    if (ext)
    {
        free_extensions(ext, counters.ext);
    }
    
    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'delete' functionality
ErrorCode parse_delete_options(int argc, char **argv, int opt_start, void *opts_out)
{
    DeleteOptions *opts = (DeleteOptions*)opts_out;

    uint32_t supported_flags = COMMON_HUMAN_READABLE |
                               COMMON_RECURSIVE |
                               FILTER_CONTAINS |
                               FILTER_EXTENSION |
                               FILTER_MAX_SIZE |
                               FILTER_MIN_SIZE |
                               FILTER_TYPE |
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
    
    return EC_SUCCESS;
}

// Deletes elements
static void delete_element(const char *current_path, DeleteOptions *opts, struct dirent *namelist,
                           Extension *ext, DeleteCounters *counters)
{
    // Creates full path to current element
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        return;
    }

    bool is_dir = (namelist->d_type == DT_DIR);
    if (!is_dir && namelist->d_type == DT_UNKNOWN)
    {
        struct stat st;
        if (lstat(full_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
            counters->error++;
            return;
        }

        is_dir = S_ISDIR(st.st_mode);
    }
    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        // Checks for directory type
        if (is_directory_type(opts->filter.type))
        {
            bool can_nuke = true;  // Deletes whole directory

            // Checks for 'contains' filter
            if (opts->filter.contains && opts->filter.contains[0] != '\0')
            {
                if (!match_name(opts->filter.contains, namelist->d_name))
                {
                    can_nuke = false;
                }
            }

            // Checks for 'max-size' and 'min-size' filters
            if (opts->filter.max_size || opts->filter.min_size)
            {
                off_t dir_size = 0;
                if (!match_directory_size(full_path, opts->filter.max_size, opts->filter.min_size, &dir_size))
                {
                    can_nuke = false;
                }
            }

            // Deletes whole directory
            if (can_nuke)
            {
                if (opts->action.interactive)
                {
                    char *prompt = NULL;
                    if (asprintf(&prompt, "Fully remove directory '%s'?", full_path) == -1)
                    {
                        fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                        free(prompt);
                        return;
                    }
                    if (!get_answer(prompt))
                    {
                        free(prompt);
                        return;
                    }

                    free(prompt);
                }
                if (opts->action.dry_run)
                {
                    printf("[DRY-RUN] Would fully remove directory %s\n", full_path);
                    counters->dlt_directories++;
                }
                else
                {
                    if (delete_directory(opts, full_path, counters))
                    {
                        if (opts->action.verbose)
                        {
                            printf("Directory completely deleted: %s\n", full_path);
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
        }

        // Recursivelly calls directory's elements
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(full_path, &entry, scandir_no_dot_filter, alphasort);
            if (n == -1)
            {
                fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
                counters->error++;
                return;
            }

            for (int i = 0; i < n; i++)
            {
                delete_element(full_path, opts, entry[i], ext, counters);
            }
            free_dirent(entry, n);
        }

        // Tries to remove current directory (if empty after recursion)
        if (opts->action.dry_run)
        {
            printf("[DRY-RUN] Would remove directory %s\n", full_path);
            counters->dlt_directories++;
        }
        else
        {
            if (rmdir(full_path) == 0)
            {
                if (opts->action.verbose)
                {
                    printf("Directory deleted: %s\n", full_path);
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
    }

    // ================== FILES & SLINKS ===================
    else
    {
        if (opts->filter.type || opts->filter.max_size || opts->filter.min_size)
        {
            struct stat st;
            if (lstat(full_path, &st) != 0)
            {
                fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
                counters->error++;
                return;
            }

            // Checks for element's type
            if (opts->filter.type && !match_type(opts->filter.type, st.st_mode))
            {
                return;
            }

            // Checks for size
            if ((opts->filter.max_size || opts->filter.min_size) &&
                !match_size(opts->filter.max_size, opts->filter.min_size, st.st_size))
            {
                return;
            }
        }

        // Checks for name equality (contains)
        if (opts->filter.contains && opts->filter.contains[0] != '\0')
        {
            if (!match_name(opts->filter.contains, namelist->d_name))
            {
                return;
            }
        }

        // Checks for matching extension
        if (ext && !match_extension(ext, counters->ext, namelist->d_name))
        {
            return;
        }

        if (opts->action.interactive)
        {
            char *prompt = NULL;
            if (asprintf(&prompt, "Delete file: '%s'?", full_path) == -1)
            {
                fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                free(prompt);
                return;
            }
            if (!get_answer(prompt))
            {
                free(prompt);
                return;
            }
            free(prompt);
        }

        struct stat st;
        if (lstat(full_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
            counters->error++;
            return;
        }
        if (opts->action.dry_run)
        {
            printf("[DRY-RUN] Would delete file %s\n", full_path);
            counters->dlt_files++;
            counters->dlt_size += st.st_size;
        }
        else
        {
            if (remove(full_path) == 0)
            {
                if (opts->action.verbose)
                {
                    printf("File deleted: %s\n", full_path);
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
}

// Fully deletes a directory
static bool delete_directory(DeleteOptions *opts, const char *path, DeleteCounters *counters)
{
    struct dirent **ptr = NULL;
    int n = scandir(path, &ptr, scandir_no_dot_filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        return false;
    }

    for (int i = 0; i < n; i ++)
    {
        char new_path[PATH_MAX];
        if (check_path_name_size(new_path, sizeof(new_path), path, ptr[i]->d_name) == -1)
        {
            fprintf(stderr, "Path too long: %s/%s\n", new_path, ptr[i]->d_name);
            continue;
        }
    
        bool is_dir = (ptr[i]->d_type == DT_DIR);
        if (!is_dir && ptr[i]->d_type == DT_UNKNOWN)
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
            delete_directory(opts, new_path, counters);
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
