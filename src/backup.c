#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>  // PATH_MAX
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>  // access()

// Headers
#include "backup.h"
#include "cli_parse.h"
#include "commands.h"
#include "help.h"
#include "log.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void backup_element(const struct dirent *namelist, BackupOptions *opts, Extension *ext,
                           const char *base_dir, const char *current_path, const char *dst_path,
                           BackupCounters *counters, ScandirFilter filter);
static int mark_destination_backup(const char *dst_dir, const char *base_dir);

// Setup logic for 'backup' feature
ErrorCode handle_backup(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_backup_help,
                                            parse_backup_opts, sizeof(BackupOptions));
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

    BackupOptions *opts = (BackupOptions*)context->opts;
    // Initializes counters
    BackupCounters counters = {0};
    
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
        backup_element(namelist[i], opts, ext, context->base_dir, context->base_dir,
                       context->dst_dir, &counters, context->filter);
    }
    free_dirent(namelist, n);

    if (!opts->action.dry_run)
    {
        // Creates flags to indicate backup directory
        mark_destination_backup(context->dst_dir, context->base_dir);
    }

    // Prints output message
    print_output_message("Backed up", opts->action, counters.bck_files);
    print_counter_err_msg(counters.error);

cleanup:
    if (ext)
    {
        free_extensions(ext, counters.ext);
    }

    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'backup' functionality
ErrorCode parse_backup_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    BackupOptions *opts = (BackupOptions*)opts_out;

    opterr = 0;

    // Supported flags
    uint32_t common_flags = COMMON_ALL |
                            COMMON_ALMOST_ALL |
                            COMMON_RECURSIVE;
    uint32_t filter_flags = FILTER_CONTAINS |
                            FILTER_EXTENSION |
                            FILTER_MAX_SIZE |
                            FILTER_MIN_SIZE;
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
        // Action flags
        {"dry-run", no_argument, 0, 'd'},
        {"interactive", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "aARc:e:div";

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

    // Validates size
    if (opts->filter.max_size == -1 || opts->filter.min_size == -1)
    {
        return EC_PARSE_ERROR_SIZE;
    }

    if (opts->base.almost_all)
    {
        opts->base.all = false;
    }

    opts->filter.supported = filter_flags;
    return EC_SUCCESS;
}

// Backs up files
static void backup_element(const struct dirent *namelist, BackupOptions *opts, Extension *ext,
                           const char *base_dir, const char *current_path, const char *dst_path,
                           BackupCounters *counters, ScandirFilter filter)
{
    // Creates full path to current element (origin)
    char src_path[PATH_MAX];
    if (check_path_name_size(src_path, sizeof(src_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        counters->error++;
        return;
    }

    // Creates path to new directory (destination)
    char new_dst_path[PATH_MAX];
    if (check_path_name_size(new_dst_path, sizeof(new_dst_path), dst_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", dst_path, namelist->d_name);
        counters->error++;
        return;
    }

    struct stat st;
    bool has_data = false;

    // Checks for directory type
    bool is_dir = (namelist->d_type == DT_DIR);
    if (namelist->d_type == DT_UNKNOWN)
    {
        if (lstat(src_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
            counters->error++;
            return;
        }

        is_dir = S_ISDIR(st.st_mode);
        has_data = true;
    }

    // Gets source's and destination's suffix (cleaner output)
    const char *src_suffix = get_suffix(src_path, base_dir);

    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        if (!has_data)
        {
            if (lstat(src_path, &st) != 0)
            {
                fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
                counters->error++;
                return;
            }
        }

        // Creates subdirectory
        if (mkdir(new_dst_path, st.st_mode & 0777) == -1 && errno != EEXIST)
        {
            fprintf(stderr, "Couldn't create directory %s: %s", new_dst_path, strerror(errno));
            counters->error++;
            return;
        }
    
        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(src_path, &entry, filter, alphasort);
            if (n == -1)
            {
                fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
                counters->error++;
                return;
            }
            for (int i = 0; i < n; i++)
            {
                backup_element(entry[i], opts, ext, base_dir, src_path,
                               new_dst_path, counters, filter);
            }
            free_dirent(entry, n);
        }

        // Tries to remove current directory (if empty after recursion)
        rmdir(new_dst_path);

        return;
    }

    // ================== FILES & SLINKS ===================
    // Checks file's validity
    if (check_file_flags(namelist, ext, src_path, &opts->filter,
                         counters->ext, &counters->error))
    {
        if (opts->action.interactive && !opts->action.dry_run)
        {
            char *prompt = NULL;
            if (asprintf(&prompt, "Backup file '%s'?", src_suffix) == -1)
            {
                fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                counters->error++;
                if (prompt)
                {
                    free(prompt);
                }
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

        if (!has_data)
        {
            if (lstat(src_path, &st) != 0)
            {
                fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
                counters->error++;
                return;
            }
        }

        if (file_needs_backup(&st, src_path, new_dst_path))
        {
            if (opts->action.dry_run)
            {
                if (opts->action.verbose)
                {
                    printf("[DRY-RUN] Would backup file '%s'\n", src_suffix);
                }
                counters->bck_files++;
            }
            else
            {
                if (copy_file(src_path, new_dst_path) == 0)
                {
                    if ( opts->action.verbose)
                    {
                        printf("Backed up file: '%s'\n", src_suffix);
                    }

                    update_log_file(LOG_SUCCESS, CMD_BACKUP, src_path, new_dst_path, false);
                    counters->bck_files++;
                }
                else
                {
                    update_log_file(LOG_ERROR, CMD_BACKUP, src_path, new_dst_path, true);
                    counters->error++;
                }
            }
        }
    }
}

// Creates hiden file to identify backup diretory (used on 'recover' feature)
static int mark_destination_backup(const char *dst_dir, const char *base_dir)
{
    char marker_path[PATH_MAX];
    char *marker = ".archivist-backup";
    if (check_path_name_size(marker_path, sizeof(marker_path), dst_dir, marker) == -1)
    {
        return -1;
    }

    // Checks if marker already exist
    if (access(marker_path, F_OK) == 0)
    {
        return 0;
    }

    // Gets time
    char timestamp[32] = {0};
    get_formatted_time(timestamp, sizeof(timestamp));
    if (timestamp[0] == '\0')
    {
        strcpy(timestamp, "N/A");
    }

    FILE* file = fopen(marker_path, "w");
    if (!file)
    {
        fprintf(stderr, "Failed to create backup marker: %s\n", strerror(errno));
        return -1;
    }

    fprintf(file,
            "archivist-backup\n"
            "created at: %s\n"
            "source: %s\n",
            timestamp, base_dir);
    
    fclose(file);
    return 0;
}
