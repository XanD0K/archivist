#define _GNU_SOURCE

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
#include "error_code.h"
#include "help.h"
#include "recover.h"
#include "utils.h"

// Prototypes
static bool check_marker(const char *base_dir);
static void recover_element(struct dirent *namelist, char *current_path, char *dst_path, RecoverOptions *opts);

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

    // Checks/Creates destination directory
    const char *dir_path_dst = (argc >= 4 && argv[3][0] != '-') ? argv[2] : argv[3];
    char *dst_dir = get_valid_destination(dir_path_dst);
    if (!dst_dir)
    {
        free_command_context(context);
        return EC_INVALID_DIRECTORY;
    }

    RecoverOptions *opts = (RecoverOptions*)context->opts;

    // Checks for backed up marker
    if (!check_marker(context->base_dir))
    {
        free_command_context(context);
        return EC_MISSING_BACKUP_MARKER;
    }

    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        return EC_SCANDIR_ERROR;
    }
    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            recover_element(namelist[i], context->base_dir, dst_dir, opts);
        }
        free(namelist[i]);
    }

    free(namelist);
    free_command_context(context);

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

static bool check_marker(const char *base_dir)
{
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), base_dir, ".archivist-backup") == -1)
    {
        return false;
    }

    if (access(full_path, F_OK) != 0)
    {
        return false;
    }

    return true;
}

static void recover_element(struct dirent *namelist, char *current_path, char *dst_path, RecoverOptions *opts)
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

    struct stat st;
    if (stat(src_path, &st) != 0)
    {
        return;
    }

    char new_dst_path[PATH_MAX];
    if (check_path_name_size(new_dst_path, sizeof(new_dst_path), dst_path, namelist->d_name) == -1)
    {
        return;
    }

    // ==================== DIRECTORIES ====================
    if (S_ISDIR(st.st_mode))
    {
        if (mkdir(new_dst_path, st.st_mode & 0777) == -1 && errno != EEXIST)
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
                recover_element(entry[i], src_path, new_dst_path, opts);
                free(entry[i]);
            }

            free(entry);
        }

        return;
    }

    // ================== FILES & SLINKS ===================
    if (opts->action.interactive)
    {
        char *prompt = NULL;
        if ((asprintf(&prompt, "Recover file %s? ", src_path)) == -1)
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
    }
    else
    {
        if (file_needs_backup(&st, new_dst_path))
        {
            if (copy_file(src_path, new_dst_path) == 0)
            {
                if (opts->action.verbose)
                {
                    printf("Recovered files %s\n", src_path);
                }
            }
            else
            {
                // Error message
            }

        }
    }
}
