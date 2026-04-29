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
static void rename_element(const struct dirent *namelist, RenameOptions *opts, Extension *ext,
                           const char *base_dir, const char *current_path, SortScandir sorter,
                           RenameCounters *counters, ScandirFilter filter);
static char *generate_unique_name(const char *current_path, const char *old_name,
                                  const char *input_name, size_t *counter);
static bool is_system_file (const char *name);

// Setup logic for 'rename' feature
ErrorCode handle_rename(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_rename_help,
                                            parse_rename_opts, sizeof(RenameOptions));
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
    SortScandir sorter = cmp_name_scandir;
    if (opts->base.sort && opts->base.sort[0] != '\0' && 
        strcasecmp(opts->base.sort, "name") != 0)
    {
        // All sort methods available
        const char *sorts[] = {"date", "size", "version"};
        size_t len = sizeof(sorts) / sizeof(sorts[0]);

        // Updates sorter function
        sorter = get_scandir_sort_fn(opts->base.sort, sorts, len);
        if (!sorter)
        {
            free_command_context(context);
            return EC_INVALID_SORTER;
        }
    }

    // Initializes counters
    RenameCounters counters = {0};
    counters.name = 1;

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

    // Determines the filter used in scandir()
    ScandirFilter filter = scandir_visible_only;
    if (opts->base.all)
    {
        filter = scandir_no_dot_filter;
    }
    else if (opts->base.almost_all)
    {
        filter = scandir_show_hidden_files;
    }

    // Retrieves directory's content
    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, filter, sorter);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        ret = EC_SCANDIR_ERROR;
        goto cleanup;
    }

    for (int i = 0; i < n; i++)
    {
        rename_element(namelist[i], opts, ext, context->base_dir, context->base_dir,
                       sorter, &counters, filter);
    }
    free_dirent(namelist, n);

    // Prints output message
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Renamed files: %zu\n", counters.rnmd_files);
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
    if (ext)
    {
        free_extensions(ext, counters.ext);
    }

    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'rename' functionality
ErrorCode parse_rename_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    RenameOptions *opts = (RenameOptions*)opts_out;
    opts->base.sort = "name";

    opterr = 0;

    // Supported flags
    uint32_t common_flags = COMMON_ALL |
                            COMMON_ALMOST_ALL |
                            COMMON_RECURSIVE |
                            COMMON_SORT;
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
        {"sort", optional_argument, 0, 's'},
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
        // Specific flags
        {"name", required_argument, 0, 'n'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "aAs::Rc:e:divn:";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // ========== COMMON ==========
            case 'a':  // all
            case 'A':  // almost-all
            case 's':  // sort
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
                handle_filter_flag(opt, long_opts[long_index].name, optarg, &opts->filter, filter_flags);
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
            case 'n':  // dry-run
            {
                opts->name = optarg;
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

    if (!opts->name || opts->name[0] == '\0')
    {
        opts->name = "file";
    }

    opts->filter.supported = filter_flags;
    return EC_SUCCESS;
}

// Renames files from given directory
static void rename_element(const struct dirent *namelist, RenameOptions *opts, Extension *ext,
                           const char *base_dir, const char *current_path, SortScandir sorter,
                           RenameCounters *counters, ScandirFilter filter)
{
    char old_path[PATH_MAX];
    if (check_path_name_size(old_path, sizeof(old_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        return;
    }

    // Gets source's suffix (cleaner output)
    const char *old_suffix = get_suffix(old_path, base_dir);

    bool is_dir = (namelist->d_type == DT_DIR);
    if (namelist->d_type == DT_UNKNOWN)
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
        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(old_path, &entry, filter, sorter);
            if (n == -1)
            {
                return;
            }

            for (int i = 0; i < n; i++)
            {
                rename_element(entry[i], opts, ext, base_dir, old_path,
                               sorter, counters, filter);
            }
            free_dirent(entry, n);
        }
        counters->name = 1;  // Resets counters for each subdirectory
        return;
    }

    // ================== FILES & SLINKS ===================
    if (check_file_flags(namelist, ext, old_path, &opts->filter, counters->ext, &counters->error))
    {
        // Gets new name
        char *new_path = generate_unique_name(current_path, namelist->d_name, opts->name, &counters->name);
        if (!new_path)
        {
            return;
        }

        // Gets new_name suffix (cleaner output)
        const char *new_suffix = get_suffix(new_path, base_dir);

        if (opts->action.interactive)
        {
            char *prompt = NULL;
            if (asprintf(&prompt, "Rename file '%s' → '%s'? ", old_path, new_suffix) == -1)
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
            if (opts->action.verbose)
            {
                printf("[DRY-RUN] Would rename file: '%s' → '%s'\n", old_suffix, new_suffix);
            }
            counters->rnmd_files++;
        }
        else
        {
            // Renames files
            if (rename(old_path, new_path) == 0)
            {
                if (is_system_file(namelist->d_name))
                {
                    printf("Renamed file: '%s' → '%s'\n"
                            "WARNING: This looks like a system or configuration file!\n", old_suffix, new_suffix);
                }
                if (opts->action.verbose)
                {
                    printf("Renamed file: '%s' → '%s'\n", old_suffix, new_suffix);
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
static char *generate_unique_name(const char *current_path, const char *old_name,
                                  const char *input_name, size_t *counter)
{
    // Default behavior: incremental rename
    const char *dot = strrchr(old_name, '.');
    const char *ext = (dot) ? dot : "";

    char new_name[PATH_MAX];
    char full_path[PATH_MAX];
    const size_t MAX_ATTEMPTS = 10000;

    for (size_t attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
    {
        snprintf(new_name, sizeof(new_name), "%s_%zu%s", input_name, (*counter), ext);

        if (check_path_name_size(full_path, sizeof(full_path), current_path, new_name) == -1)
        {
            return NULL;
        }

        if (access(full_path, F_OK) != 0)
        {
            (*counter)++;
            return strdup(full_path);
        }

        (*counter)++;

    }

    fprintf(stderr, "Could not find an available name for '%s'\n", old_name);
    return NULL;
}

// Identifies system's files
static bool is_system_file (const char *name)
{
    const char *ext = get_clean_extension(name);
    if (!ext || ext[0] == '\0')
    {
        return false;
    }

    const char *dangerous_exts[] = {
    // Configuração
    ".ini", ".conf", ".cfg", ".config", ".yaml", ".yml", ".json", ".xml",
    
    // Logs e temporários
    ".log", ".bak", ".tmp", ".swp", ".old", ".orig",
    
    // Sistema / Daemon
    ".pid", ".lock", ".service", ".socket", ".timer", ".mount",
    
    // Ambiente e scripts
    ".env", ".sh", ".bash", ".zsh", ".rc", ".profile",
    
    // Específicos do Linux / Desktop
    ".desktop", ".htaccess", ".htpasswd", ".gitignore", ".gitattributes",
    ".editorconfig", ".npmrc", ".yarnrc",
    
    NULL
    };

    for (size_t i = 0; dangerous_exts[i]; i++)
    {
        if (strcasestr(name, dangerous_exts[i]))
        {
            return true;
        }
    }
    return false;
}
