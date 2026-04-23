#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>  // access()

// Headers
#include "cli_parse.h"
#include "commands.h"
#include "help.h"
#include "log.h"
#include "recover.h"
#include "utils.h"

// Prototypes
static bool check_marker(const char *base_dir);
static void recover_element(struct dirent *namelist, char *current_path, char *dst_path,
                            RecoverOptions *opts, RecoverCounters *counters);

// Setup logic for 'recover' feature
ErrorCode handle_recover(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_recover_help, parse_recover_options, sizeof(RecoverOptions));
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

    RecoverOptions *opts = (RecoverOptions*)context->opts;
    RecoverCounters counters = {0};

    // Checks for backed up directory marker
    if (!check_marker(context->base_dir))
    {
        free_command_context(context);
        return EC_MISSING_BACKUP_MARKER;
    }

    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, scandir_no_dot_filter, alphasort);
    if (n == -1)
    {
        return EC_SCANDIR_ERROR;
    }
    for (int i = 0; i < n; i++)
    {
        recover_element(namelist[i], context->base_dir, context->dst_dir, opts, &counters);
    }
    free_dirent(namelist, n);
    free_command_context(context);

    // Output Message
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Would recover %zu files\n", counters.rcv_files);
    }
    else
    {
        printf("Recovered files: %zu\n", counters.rcv_files);
    }

    if (counters.error != 0)
    {
        printf("(Finished with %zu errors)\n", counters.error);
    }

    return EC_SUCCESS;
}

// Parses through CLI arguments for 'recover' functionality
ErrorCode parse_recover_options(int argc, char **argv, int opt_start, void *opts_out)
{
    RecoverOptions *opts = (RecoverOptions*)opts_out;

    uint32_t supported_flags = COMMON_HUMAN_READABLE |
                               ACTION_DRY_RUN |
                               ACTION_INTERACTIVE |
                               ACTION_VERBOSE;

    ErrorCode ret;
    ret = parse_common_opts(argc, argv, opt_start, &opts->base, supported_flags);
    if (ret != EC_SUCCESS)
    {
        return ret;
    }

    ret = parse_action_options(argc, argv, opt_start, &opts->action, supported_flags);
    if (ret != EC_SUCCESS)
    {
        return ret;
    }
    
    return EC_SUCCESS;
}

// Checks if directory has backup marker
static bool check_marker(const char *base_dir)
{
    char marker_path[PATH_MAX];
    if (check_path_name_size(marker_path, sizeof(marker_path), base_dir, ".archivist-backup") == -1)
    {
        return false;
    }

    if (access(marker_path, F_OK) != 0)
    {
        return false;
    }

    return true;
}

// Recovers files from backup directory
static void recover_element(struct dirent *namelist, char *current_path, char *dst_path,
                            RecoverOptions *opts, RecoverCounters *counters)
{
    char src_path[PATH_MAX];
    if (check_path_name_size(src_path, sizeof(src_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        return;
    }

    char new_dst_path[PATH_MAX];
    if (check_path_name_size(new_dst_path, sizeof(new_dst_path), dst_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", dst_path, namelist->d_name);
        return;
    }

    bool is_dir = (namelist->d_type == DT_DIR);
    if (!is_dir && namelist->d_type == DT_UNKNOWN)
    {
        struct stat st;
        if (lstat(src_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
            return;
        }

        is_dir = S_ISDIR(st.st_mode);
    }
    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        struct stat st;
        if (lstat(src_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
            counters->error++;
            return;
        }

        if (mkdir(new_dst_path, st.st_mode & 0777) == -1 && errno != EEXIST)
        {
            fprintf(stderr, "Couldn't create directory %s: %s", new_dst_path, strerror(errno));
            counters->error++;
            return;
        }

        struct dirent **entry;
        int n = scandir(src_path, &entry, scandir_no_dot_filter, alphasort);
        if (n == -1)
        {
            return;
        }
        for (int i = 0; i < n; i++)
        {
            recover_element(entry[i], src_path, new_dst_path, opts, counters);
        }
        free_dirent(entry, n);

        return;
    }

    // ================== FILES & SLINKS ===================
    if (opts->action.interactive)
    {
        char *prompt = NULL;
        if ((asprintf(&prompt, "Recover file %s?", src_path)) == -1)
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
        printf("[DRY-RUN] Would recover file %s\n", src_path);
        counters->rcv_files++;
    }
    else
    {
        struct stat st;
        if (lstat(src_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
            counters->error++;
            return;
        }
        if (file_needs_backup(&st, new_dst_path))
        {
            if (copy_file(src_path, new_dst_path) == 0)
            {
                if (opts->action.verbose)
                {
                    printf("File '%s' recovered to '%s'\n", namelist->d_name, new_dst_path);
                }

                update_log_file(LOG_SUCCESS, CMD_RECOVER, src_path, new_dst_path, false);
                counters->rcv_files++;
            }
            else
            {
                fprintf(stderr, "Couldn't recover %s: %s\n", src_path, strerror(errno));
                update_log_file(LOG_ERROR, CMD_RECOVER, src_path, new_dst_path, true);
                counters->error++;
            }
        }
    }
}
