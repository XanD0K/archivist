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
#include "log.h"
#include "move.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void move_element(const struct dirent *namelist, MoveOptions *opts, Extension *ext,
                         const char *base_dir, char *current_path, char *dst_dir,
                         MoveCounters *counters, ScandirFilter filter, bool nuke_move);
static char *match_existed_file(const char *dst_dir, const char *new_dst_dir,
                                const char *name, bool skip, bool force);

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
        return (context->error_code == EC_CMD_HELP_FLAG)
            ? EC_SUCCESS
            : context->error_code;
    }

    MoveOptions *opts = (MoveOptions*)context->opts;

    // Initializes counters
    MoveCounters counters = {0};
    // Default return value
    ErrorCode ret = EC_SUCCESS;

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
        move_element(namelist[i], opts, ext, context->base_dir, context->base_dir,
                     context->dst_dir, &counters, context->filter, false);
    }
    free_dirent(namelist, n);

    // Prints output message
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Moved files: %zu\n"
               "[DRY-RUN] Moved directories: %zu\n",
               counters.moved_files, counters.moved_directories);
    }
    else
    {
        printf("Moved files: %zu\n"
               "Moved directories: %zu\n", counters.moved_files, counters.moved_directories);
    }
    print_counter_err_msg(counters.error);

cleanup:
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
    uint32_t action_flags = ACTION_DRY_RUN |
                            ACTION_INTERACTIVE |
                            ACTION_VERBOSE;

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
        // Action flags
        {"dry-run", no_argument, 0, 'd'},
        {"interactive", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        // Specific flags
        {"force", no_argument, 0, 'f'},
        {"skip", no_argument, 0, 'S'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "aARc:e:t:divfS";

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
            // ========== ACTION ==========
            case 'd':  // dry-run
            case 'i':  // interactive
            case 'v':  // verbose
            {
                handle_action_flag(opt, &opts->action, action_flags);
                break;
            }
            // ========= SPECIFIC =========
            case 'f':  // force
            {
                opts->force = true;
                opts->skip = false;
                break;
            }
            case 'S':  // skip
            {
                opts->force = false;
                opts->skip = true;
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

    // Defines behaviour when already existed file on destination
    if (opts->skip)
    {
        opts->force = false;
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

// Moves files from one directory to another
static void move_element(const struct dirent *namelist, MoveOptions *opts, Extension *ext,
                         const char *base_dir, char *current_path, char *dst_dir,
                         MoveCounters *counters, ScandirFilter filter, bool nuke_move)
{
    // Creates full path to current element (origin)
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        counters->error++;
        return;
    }

    // Creates path to new directory (destination)
    char new_dst_dir[PATH_MAX];
    if (check_path_name_size(new_dst_dir, sizeof(new_dst_dir), dst_dir, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", dst_dir, namelist->d_name);
        counters->error++;
        return;
    }

    // Checks for directory type
    bool is_dir = (namelist->d_type == DT_DIR);
    if (namelist->d_type == DT_UNKNOWN)
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
    
    // Gets source's and destination's suffix (cleaner output)
    const char *src_suffix = get_suffix(full_path, base_dir);

    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        // Checks directory's validity
        if (check_directory_flags(namelist, full_path, &opts->filter, filter))
        {
            // Gets user's confirmation before moving directory
            if (!nuke_move && opts->action.interactive && !opts->action.dry_run)
            {
                char *prompt = NULL;
                if (asprintf(&prompt, "Fully move directory '%s'?", src_suffix) == -1)
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

                nuke_move = true;
                free(prompt);
            }

            if (opts->action.dry_run)
            {
                printf("[DRY-RUN] Would move whole directory '%s'\n", src_suffix);
                counters->moved_directories++;
                nuke_move = true;
                return;
            }
            else
            {
                // Checks if destination already exists
                if (access(new_dst_dir, F_OK) != 0)
                {
                    // Creates new subdirectory on destination
                    if (rename(full_path, new_dst_dir) == 0)
                    {
                        if (opts->action.verbose)
                        {
                            printf("Fully moved directory: '%s'\n", src_suffix);
                        }
                        counters->moved_directories++;
                        update_log_file(LOG_SUCCESS, CMD_MOVE, namelist->d_name, dst_dir, false);
                    }
                    else
                    {
                        counters->error++;
                        update_log_file(LOG_ERROR, CMD_MOVE, namelist->d_name, dst_dir, true);
                    }

                    return;
                }
                else
                {
                    printf("Destination '%s' already exist! Merging contents...\n", src_suffix);
                }
            }
        }

        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            bool created = false;

            if (!opts->action.dry_run)
            {
                // Checks if directory already exists
                if (access(new_dst_dir, F_OK) != 0)
                {
                    // Creates subdirectory to move its files
                    if (mkdir(new_dst_dir, 0755) != 0)
                    {
                        fprintf(stderr, "Failed to create directory %s: %s\n", new_dst_dir, strerror(errno));
                        counters->error++;
                        return;
                    }

                    created = true;

                    if (opts->action.verbose)
                    {
                        printf("Directory moved: '%s'\n", src_suffix);
                    }
                }
            }

            // Retrieves all content from base directory
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
                move_element(entry[i], opts, ext, base_dir, full_path,
                             new_dst_dir, counters, filter, nuke_move);
            }
            free_dirent(entry, n);

            // Directory was created
            if (created && !opts->action.dry_run)
            {
                // Tries to remove destination directory (if empty after recursion)
                if (rmdir(new_dst_dir) != 0 && errno == ENOTEMPTY)
                {
                    counters->moved_directories++;
                    update_log_file(LOG_SUCCESS, CMD_MOVE, current_path, dst_dir, false);
                }
            }

            // Tries to remove source directory (if empty after recursion)
            rmdir(full_path);
        }

        return;
    }

    // ================== FILES & SLINKS ===================
    if (!nuke_move)
    {
        // Checks file's validity
        if (!check_file_flags(namelist, ext, full_path, &opts->filter,
                              counters->ext, &counters->error))
        {
            return;
        }
    }

    // Checks for already existed file (force, skip)
    bool existed = access(new_dst_dir, F_OK) == 0;
    char *final_dst = NULL;
    if (existed)
    {
        final_dst = match_existed_file(dst_dir, new_dst_dir, namelist->d_name,
                                       opts->skip, opts->force);
        if (!final_dst)
        {
            counters->error++;
            return;
        }
    }
    else
    {
        final_dst = strdup(new_dst_dir);
    }

    // Gets user's confirmation before moving file
    if (!nuke_move && opts->action.interactive && !opts->action.dry_run)
    {
        char *prompt = NULL;
        if (asprintf(&prompt, "Move file '%s'?", src_suffix) == -1)
        {
            fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
            counters->error++;
            free(final_dst);
            if (prompt)
            {
                free(prompt);
            }
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
        if (opts->action.verbose)
        {
            printf("[DRY-RUN] Would move file: '%s'\n", src_suffix);
        }

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
                printf("Moved file: '%s'\n", src_suffix);
            }

            counters->moved_files++;
            update_log_file(LOG_SUCCESS, CMD_MOVE, current_path, dst_dir, false);
        }
        else
        {
            counters->error++;
            update_log_file(LOG_ERROR, CMD_MOVE, current_path, dst_dir, true);
        }
    }

    free(final_dst);
}

// Defines behaviour when file already exists on destination
static char *match_existed_file(const char *dst_dir, const char *new_dst_dir,
                                const char *name, bool skip, bool force)
{
    // (-s|--skip)
    if (skip)
    {
        return NULL;
    }

    // (-f|--force)
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
    const size_t MAX_ATTEMPTS = 10000;

    for (size_t counter = 1; counter < MAX_ATTEMPTS; counter++)
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
    }

    fprintf(stderr, "Could not find an available name for '%s'\n", name);
    return NULL;
}
