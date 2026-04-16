#define _GNU_SOURCE

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>  // access()

// Headers
#include "backup.h"
#include "cli_parse.h"
#include "commands.h"
#include "help.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static int mark_destination_backup(const char *dst_dir, const char *base_dir);
static void backup_element(struct dirent *namelist, Extension *ext, size_t ext_counter,
                           const char *current_path, char *crnt_dst_path, BackupOptions *opts);

ErrorCode handle_backup(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_backup_help,
                                            parse_backup_options, sizeof(BackupOptions));
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

    // Checks/Creates destination directory
    const char *dir_path_dst = (argc >= 4 && argv[3][0] != '-') ? argv[2] : argv[3];
    char *dst_dir = get_valid_destination(dir_path_dst);
    if (!dst_dir)
    {
        return EC_INVALID_DIRECTORY;
    }

    BackupOptions *opts = (BackupOptions*)context->opts;
    
    Extension *ext = NULL;
    size_t ext_counter = 0;
    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        ext = get_all_extensions(opts->filter.extension, &ext_counter);
        if (!ext)
        {
            free_command_context(context);
            errno = ENOMEM;
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            return EC_MEMORY_ALLOCATION;
        }
    }

    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        free_extensions(ext, ext_counter);
        free_command_context(context);
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        return EC_SCANDIR_ERROR;
    }

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            backup_element(namelist[i], ext, ext_counter, context->base_dir, dst_dir, opts);
        }
        free(namelist[i]);
    }

    free(namelist);

    // Creates flags to indicate destination directory
    mark_destination_backup(dst_dir, context->base_dir);

    free_command_context(context);
    return EC_SUCCESS;
}

// Parses through CLI arguments for 'backup' functionality
ErrorCode parse_backup_options(int argc, char **argv, int opt_start, void *opts_out)
{
    BackupOptions *opts = (BackupOptions*)opts_out;

    uint32_t supported_flags = COMMON_HUMAN_READABLE |
                               COMMON_RECURSIVE |
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
    
    return EC_SUCCESS;
}

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
    time_t now = time(NULL);  // Current time in seconds (since 1970)
    struct tm *tm_info = localtime(&now);  // Converts to local format
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%F %T", tm_info);

    FILE* file = fopen(marker_path, "w");
    if (!file)
    {
        fprintf(stderr, "Failed to create backup marker: %s", strerror(errno));
        return -1;
    }

    fprintf(file,
            "archivist-backup\n"
            "created ai: %s\n"
            "source: %s\n",
            timestamp, base_dir);
    
    fclose(file);
    return 0;
}

// Backs up files
static void backup_element(struct dirent *namelist, Extension *ext, size_t ext_counter,
                           const char *current_path, char *crnt_dst_path, BackupOptions *opts)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    char src_path[PATH_MAX];
    if (check_path_name_size(src_path, sizeof(src_path), current_path, namelist->d_name) == -1)
    {
        return;
    }

    struct stat st_src;
    if (stat(src_path, &st_src) != 0)
    {
        return;
    }

    char new_dst_path[PATH_MAX];
    if (check_path_name_size(new_dst_path, sizeof(new_dst_path), crnt_dst_path, namelist->d_name) == -1)
    {
        return;
    }

    // ==================== DIRECTORIES ====================
    if (S_ISDIR(st_src.st_mode))
    {
        // Creates subdirectory
        if (mkdir(new_dst_path, st_src.st_mode & 0777) == -1 && errno != EEXIST)
        {
            return;
        }
    
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(src_path, &entry, NULL, alphasort);
            if (n == -1)
            {
                return;
            }
            for (int i = 0; i < n; i++)
            {
                backup_element(entry[i], ext, ext_counter, src_path, new_dst_path, opts);
                free(entry[i]);
            }

            free(entry);
        }

        return;
    }

    // ================== FILES & SLINKS ===================
    if (opts->filter.contains && opts->filter.contains[0] != '\0')
    {
        if (!match_name(opts->filter.contains, namelist->d_name))
        {
            return;
        }
    }

    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        if (!match_extension(ext, ext_counter, namelist->d_name))
        {
            return;
        }
    }

    if ((opts->filter.max_size || opts->filter.min_size) &&
        match_size(opts->filter.max_size, opts->filter.min_size, st_src.st_size))
    {
        return;
    }

    if (opts->action.interactive)
    {
        char *prompt = NULL;
        if (asprintf(&prompt, "Backup file %s? ", src_path) == -1)
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
        printf("[DRY-RUN] Would backup file %s\n", src_path);
    }
    else
    {
        // Checks if current files has changed
        if (file_needs_backup(&st_src, new_dst_path))
        {
            if (copy_file(src_path, new_dst_path) == 0)
            {
                if( opts->action.verbose)
                {
                    printf("File %s backed up to %s\n", src_path, new_dst_path);
                }
            }
            else
            {
                // ERROR MESSAGE
            }
        }
    }
}
