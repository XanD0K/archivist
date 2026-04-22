#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>  // PAT_MAX
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
#include "log.h"
#include "move.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void move_element(char *current_path, char *dst_dir, MoveOptions *opts,
                         struct dirent *namelist, Extension *ext, MoveCounters *counters);
static char *match_existed_file(char *dst_dir, char *new_dst_dir, char *name, bool skip, bool force);

// Setup logic for 'move' feature
ErrorCode handle_move(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_move_help,
                                            parse_move_opts, sizeof(MoveOptions));
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

    MoveOptions *opts = (MoveOptions*)context->opts;

    // Initializes counters
    MoveCounters counters = {0};
    // Default return value
    ErrorCode ret = EC_SUCCESS;

    // Retrieves user's selected extensions (-e|--extension flag)
    Extension *ext = NULL;
    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        ext = get_all_extensions(opts->filter.extension, &counters.ext);
        if (!ext)
        {
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            ret = EC_MEMORY_ALLOCATION;
            goto cleanup;
        }
    }
    
    // Retrieves directory's content
    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, scandir_no_dot_filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        ret = EC_SCANDIR_ERROR;
        goto cleanup;
    }

    for (int i = 0; i < n; i++)
    {
        move_element(context->base_dir, context->dst_dir, opts, namelist[i], ext, &counters);
    }

    // Output Message
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Files that would be moved: %zu\n"
               "[DRY-RUN] Directories that would be created: %zu\n",
               counters.moved_files, counters.moved_directories);
    }
    else
    {
        printf("Moved files: %zu\n"
               "Created directories: %zu\n", counters.moved_files, counters.moved_directories);
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

// Parses through CLI arguments for 'move' functionality
ErrorCode parse_move_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    MoveOptions *opts = (MoveOptions*)opts_out;

    uint32_t supported_flags = COMMON_RECURSIVE |
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

    static struct option long_opts[] =
    {
        {"force", no_argument, 0, 'f'},
        {"skip", no_argument, 0, 's'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "fs";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'f':
            {
                opts->force = true;
                opts->skip = false;
                break;
            }
            case 's':
            {
                opts->force = false;
                opts->skip = true;
                break;
            }
            case '?':
            {
                fprintf(stderr, "Flag not allowed: %s\n", argv[optind - 1]);
                return EC_PARSE_ERROR;
            }
        }
    }
    
    return EC_SUCCESS;
}

// Move files from one directory to another
static void move_element(char *current_path, char *dst_dir, MoveOptions *opts,
                         struct dirent *namelist, Extension *ext, MoveCounters *counters)
{
    // Creates full path to current element (origin)
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        return;
    }

    // Creates path to new directory (destination)
    char new_dst_dir[PATH_MAX];
    if (check_path_name_size(new_dst_dir, sizeof(new_dst_dir), dst_dir, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", dst_dir, namelist->d_name);
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
        // Checks for whole directory to be moved
        if (is_directory_type(opts->filter.type))
        {
            bool can_move_whole = true;

            // Checks for name equality (contains)
            if (opts->filter.contains && opts->filter.contains[0] != '\0')
            {
                if (!match_name(opts->filter.contains, namelist->d_name))
                {
                    can_move_whole = false;
                }
            }

            if (can_move_whole && (opts->filter.max_size || opts->filter.min_size))
            {
                off_t dir_size = 0;
                if (!match_directory_size(full_path, opts->filter.max_size, opts->filter.min_size, &dir_size))
                {
                    can_move_whole = false;
                }
            }

            if (can_move_whole)
            {                
                // Gets user's confirmation before moving directory
                if (opts->action.interactive)
                {
                    char *prompt = NULL;
                    if (asprintf(&prompt, "Moves directory %s?", new_dst_dir) == -1)
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
                    printf("[DRY-RUN] Would move directory %s\n", new_dst_dir);
                    counters->moved_directories++;
                }
                else
                {
                    // Creates new subdirectory on destination
                    if (rename(full_path, new_dst_dir) == 0)
                    {
                        if (opts->action.verbose)
                        {
                            printf("Directory %s moved to %s\n", namelist->d_name, dst_dir);
                        }

                        update_log_file(LOG_SUCCESS, CMD_MOVE, namelist->d_name, dst_dir, false);
                        counters->moved_directories++;
                    }
                    else
                    {
                        update_log_file(LOG_ERROR, CMD_MOVE, namelist->d_name, dst_dir, true);
                        counters->error++;
                    }
                }

                return;
            }
        }

        if (opts->base.recursive)
        {
            bool created = false;
            if (opts->action.dry_run)
            {
                printf("[DRY-RUN] Would move directory %s\n", new_dst_dir);
                counters->moved_directories++;
            }
            else
            {
                // Creates subdirectory to move its files
                if (mkdir(new_dst_dir, 0755) != 0 && errno != EEXIST)
                {
                    fprintf(stderr, "Failed to create directory %s: %s\n", new_dst_dir, strerror(errno));
                    return;
                }

                if (opts->action.verbose)
                {
                    printf("Directory moved: %s\n", new_dst_dir);
                }

                created = true;
            }

            // Retrieves all content from base directory
            struct dirent **entry;
            int n = scandir(full_path, &entry, scandir_no_dot_filter, alphasort);
            if (n == -1)
            {
                return;
            }

            for (int i = 0; i < n; i++)
            {
                move_element(full_path, new_dst_dir, opts, entry[i], ext, counters);
            }

            free_dirent(entry, n);

            // Directory was created
            if (created)
            {
                // Removes created directory if empty
                if (rmdir(new_dst_dir) == 0)
                {
                    if (opts->action.verbose)
                    {
                        printf("Removed empty directory: %s\n", new_dst_dir);
                    }
                }
                // Increses counter
                else if (errno == ENOTEMPTY)
                {
                    counters->moved_directories++;
                }
                else
                {
                    counters->moved_directories++;
                    fprintf(stderr, "Could not remove directory %s: %s\n", new_dst_dir, strerror(errno));
                }
            }
        }

        return;
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

        // Checks for extension
        if (opts->filter.extension && opts->filter.extension[0] != '\0')
        {
            if (ext && !match_extension(ext, counters->ext, namelist->d_name))
            {
                return;
            }
        }

        // Checks for already existed file (force, skip)
        bool existed = access(new_dst_dir, F_OK) == 0;
        char *final_dst = NULL;
        if (existed)
        {
            final_dst = match_existed_file(dst_dir, new_dst_dir, namelist->d_name, opts->skip, opts->force);
            if (!final_dst)
            {
                return;
            }
        }
        else
        {
            final_dst = strdup(new_dst_dir);
        }

        // Gets user's confirmation before moving file
        if (opts->action.interactive)
        {
            char *prompt = NULL;
            if (asprintf(&prompt, "Move file '%s' from %s to %s?", namelist->d_name, current_path, final_dst) == -1)
            {
                fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                free(final_dst);
                free(prompt);
                return;
            }
            if (!get_answer(prompt))
            {
                free(final_dst);
                free(prompt);
                return;
            }

            free(prompt);
        }

        if (opts->action.dry_run)
        {
            printf("[DRY-RUN] Would move file from '%s' to '%s'\n", full_path, final_dst);
            counters->moved_files++;
        }
        else
        {
            // Moves file from source to destination
            if (rename(full_path, final_dst) == 0)
            {
                // Prints action in terminal
                if (opts->action.verbose)
                {
                    printf("File moved from '%s' to '%s'\n", full_path, final_dst);
                }

                update_log_file(LOG_SUCCESS, CMD_MOVE, current_path, dst_dir, false);
                counters->moved_files++;
            }
            else
            {
                update_log_file(LOG_ERROR, CMD_MOVE, current_path, dst_dir, true);
                counters->error++;
            }
        }

        free(final_dst);
    }
}

// Defines behaviour when file already exists on destination
static char *match_existed_file(char *dst_dir, char *new_dst_dir, char *name, bool skip, bool force)
{
    if (skip)
    {
        return NULL;
    }

    if (force)
    {
        remove(new_dst_dir);
        return strdup(new_dst_dir);
    }

    // Default behavior: incremental rename
    const char *dot = strrchr(name, '.');
    const char *ext = (dot) ? dot : "";

    char base[PATH_MAX];
    if (dot)
    {
        ptrdiff_t len = dot - name;  // strlen(name) - strlen(ext);
        strncpy(base, name, (size_t)len);
        base[len] = '\0';
    }
    else
    {
        strcpy(base, name);
    }

    char new_name[PATH_MAX];
    char final_path[PATH_MAX];
    size_t counter = 1;

    while (1)
    {
        snprintf(new_name, sizeof(new_name), "%s_%zu%s", base, counter, ext);

        if (check_path_name_size(final_path, sizeof(final_path), dst_dir, new_name) == -1)
        {
            return NULL;
        }

        if (access(final_path, F_OK) != 0)
        {
            return strdup(final_path);
        }

        counter++;
    }
}
