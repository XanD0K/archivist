#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

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
static void recover_element(const struct dirent *namelist, const char *base_dir,
                            const char *current_path, const char *dst_path,
                            RecoverOptions *opts, RecoverCounters *counters);

// Setup logic for 'recover' feature
ErrorCode handle_recover(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_recover_help,
                                            parse_recover_opts, sizeof(RecoverOptions));
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
    // Initializes counters
    RecoverCounters counters = {0};

    // Checks for backed up directory marker
    if (!check_marker(context->base_dir))
    {
        fprintf(stderr, "That's not a valid directory! Missing '.archivist-backup' marker!\n");
        free_command_context(context);
        return EC_MISSING_BACKUP_MARKER;
    }

    // Retrieves directory's content
    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, scandir_no_dot_filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        free_command_context(context);
        return EC_SCANDIR_ERROR;
    }

    for (int i = 0; i < n; i++)
    {
        recover_element(namelist[i], context->base_dir, context->base_dir,
                        context->dst_dir, opts, &counters);
    }
    free_dirent(namelist, n);
    free_command_context(context);

    // Prints output message
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Recovered files: %zu\n", counters.rcv_files);
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
ErrorCode parse_recover_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    RecoverOptions *opts = (RecoverOptions*)opts_out;

    opterr = 0;

    uint32_t action_flags = ACTION_DRY_RUN |
                            ACTION_INTERACTIVE |
                            ACTION_VERBOSE;

    static struct option long_opts[] =
    {
        // Action flags
        {"dry-run", no_argument, 0, 'd'},
        {"interactive", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "div";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
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
static void recover_element(const struct dirent *namelist, const char *base_dir,
                            const char *current_path, const char *dst_path,
                            RecoverOptions *opts, RecoverCounters *counters)
{
    // Filters backup marker
    if (strcmp(namelist->d_name, ".archivist-backup") == 0)
    {
        return;
    }

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

    struct stat st;
    bool has_data = false;

    bool is_dir = (namelist->d_type == DT_DIR);
    if (namelist->d_type == DT_UNKNOWN)
    {
        if (lstat(src_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
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
                fprintf(stderr, "Couldn't access '%s': %s\n", src_path, strerror(errno));
                return;
            }
        }

        if (mkdir(new_dst_path, (st.st_mode & 0777)) == -1 && errno != EEXIST)
        {
            fprintf(stderr, "Couldn't create directory '%s': %s", new_dst_path, strerror(errno));
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
            recover_element(entry[i], base_dir, src_path, new_dst_path, opts, counters);
        }
        free_dirent(entry, n);

        return;
    }

    // ================== FILES & SLINKS ===================
    if (opts->action.interactive && !opts->action.dry_run)
    {
        char *prompt = NULL;
        if ((asprintf(&prompt, "Recover file '%s'?", src_suffix)) == -1)
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

    if (!has_data)
    {
        if (lstat(src_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", src_path, strerror(errno));
            return;
        }
    }

    if (file_needs_backup(&st, src_path, new_dst_path))
    {
        if (opts->action.dry_run)
        {
            if (opts->action.verbose)
            {
                printf("[DRY-RUN] Would recover file: '%s'\n", src_suffix);
            }
            
            counters->rcv_files++;
        }
        else
        {
            if (copy_file(src_path, new_dst_path) == 0)
            {
                if (opts->action.verbose)
                {
                    printf("Recovered files: '%s'\n", src_suffix);
                }

                update_log_file(LOG_SUCCESS, CMD_RECOVER, src_path, new_dst_path, false);
                counters->rcv_files++;
            }
            else
            {
                if (opts->action.verbose)
                {
                    fprintf(stderr, "Couldn't recover file '%s': %s\n", src_suffix, strerror(errno));
                }
                
                update_log_file(LOG_ERROR, CMD_RECOVER, src_path, new_dst_path, true);
                counters->error++;
            }
        }
    }
}
