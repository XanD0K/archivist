#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>  // intmax_t
#include <limits.h>  // PATH_MAX
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
#include "help.h"
#include "list.h"
#include "utils.h"
#include "utils_sort.h"

// Prototypes
static void list_element(const struct dirent *namelist, ListOptions *opts, 
                         const char *base_dir, const char *current_path,
                         ListCounters *counters, SortScandir sorter, ScandirFilter filter);

// Setup logic for 'list' feature
ErrorCode handle_list(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_list_help,
                                            parse_list_opts, sizeof(ListOptions));
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

    ListOptions *opts = (ListOptions*)context->opts;
    // Redefines values for comparation functions (utils_sort.c)
    cmp_opts.reverse = (opts->reverse) ? -1 : 1;
    cmp_opts.dir_first = opts->dir_first;
    cmp_opts.ignore_case = opts->ignore_case;
    cmp_opts.base_dir = context->base_dir;

    // Default return value
    ErrorCode ret = EC_SUCCESS;

    // Default sorter function (by name)
    SortScandir sorter = cmp_name_scandir;
    // Checks for 'sort' flag
    if (opts->base.sort && opts->base.sort[0] != '\0' && 
        strcasecmp(opts->base.sort, "name") != 0)
    {
        // All sort methods available
        const char *sorts[] = {"date", "extension", "size", "version"};
        size_t len = sizeof(sorts)/sizeof(sorts[0]);
        
        // Updates sorter function
        sorter = get_scandir_sort_fn(opts->base.sort, sorts, len);  
        if (!sorter)
        {
            ret = EC_INVALID_SORTER;
            goto cleanup;
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
    int n = scandir(context->base_dir, &namelist, context->filter, sorter);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        ret = EC_SCANDIR_ERROR;
        goto cleanup;
    }

    // Initializes counters
    ListCounters counters = {0};

    for (int i = 0; i < n; i++)
    {
        // Recursively checks directory elements and updates counters
        list_element(namelist[i], opts, context->base_dir, context->base_dir,
                     &counters, sorter, context->filter);
    }
    free_dirent(namelist, n);

    // Prints output message
    print_divider();
    printf("Directories: %zu\n"
           "Files: %zu\n"
           "Symbolic Links: %zu\n"
           "Others: %zu\n",
           counters.directories, counters.files, counters.slinks, counters.others);
    
    if (opts->base.human_readable)
    {
        char *f_out = formatted_output(counters.total_size);
        printf("Total size: %s\n", f_out);
        free(f_out);
    }
    else
    {
        printf("Total size: %jd bytes\n", (intmax_t)(counters.total_size));
    }
    print_counter_err_msg(counters.error);

// Cleans allocated memory
cleanup:
    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'list' functionality
ErrorCode parse_list_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    // Declares structure
    ListOptions *opts = (ListOptions*)opts_out;
    opts->base.sort = "name";

    opterr = 0;

    // Supported flags
    uint32_t common_flags = COMMON_ALL |
                            COMMON_ALMOST_ALL |
                            COMMON_HUMAN_READABLE |
                            COMMON_RECURSIVE |
                            COMMON_SORT;

    static struct option long_opts[] =
    {
        // Common flags
        {"all", no_argument, 0, 'a'},
        {"almost-all", no_argument, 0, 'A'},
        {"human-readable", no_argument, 0, 'h'},
        {"sort", required_argument, 0, 's'},
        {"recursive", no_argument, 0, 'R'},
        // Specific flags
        {"reverse", no_argument, 0, 'r'},
        {"dir-first", no_argument, 0, 'D'},
        {"ignore-case", no_argument, 0, 'I'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "aAhs:RrDI";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // ========== COMMON ==========
            case 'a':  // all
            case 'A':  // almost-all
            case 'h':  // human-readable
            case 's':  // sort
            case 'R':  // recursive
            {
                handle_common_flag(opt, optarg, &opts->base, common_flags);
                break;
            }
            // ========= SPECIFIC =========
            case 'r':  // reverse
            {
                opts->reverse = true;
                break;
            }
            case 'D':  // dir-first
            {
                opts->dir_first = true;
                break;
            }
            case 'I':  // ignore-case
            {
                opts->ignore_case = true;
                break;
            }
            // ========== ERROR ===========
            case '?':
            {
                fprintf(stderr, "Flag not allowed: %s\n", argv[optind - 1]);
                return EC_PARSE_ERROR_UNSUPPORTED;
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

    return EC_SUCCESS;
}

// Lists current element and updates counters
static void list_element(const struct dirent *namelist, ListOptions *opts, 
                         const char *base_dir, const char *current_path,
                         ListCounters *counters, SortScandir sorter, ScandirFilter filter)
{
    // Builds full path to current element
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        counters->error++;
        return;
    }

    // Gets path's suffix
    const char *suffix = get_suffix(full_path, base_dir);

    // Checks for content type
    bool is_dir = (namelist->d_type == DT_DIR);
    bool is_file = (namelist->d_type == DT_REG);
    bool is_slink = (namelist->d_type == DT_LNK);

    struct stat st;
    bool has_data = false;

    if (namelist->d_type == DT_UNKNOWN || (!is_dir && !is_file && !is_slink))
    {
        // Gets element's data
        if (lstat(full_path, &st) != 0)
        {
            fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
            counters->error++;
            return;
        }
        is_dir = S_ISDIR(st.st_mode);
        is_file = S_ISREG(st.st_mode);
        is_slink = S_ISLNK(st.st_mode);

        has_data = true;
    }

    // ==================== DIRECTORIES ====================
    if (is_dir)
    {
        counters->directories++;

        // Prints directory's name
        printf("%s/\n", suffix);

        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            struct dirent **entry = NULL;
            int n = scandir(full_path, &entry, filter, sorter);
            if (n == -1)
            {
                fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
                counters->error++;
                return;
            }
            else
            {
                for (int i = 0; i < n; i++)
                {
                    // Recursively checks directory elements and updates counters
                    list_element(entry[i], opts, base_dir, full_path,
                                 counters, sorter, filter);
                }
                free_dirent(entry, n);
            }
        }
    }
    // ====================== SLINKS =======================
    else if (is_slink)
    {
        // Prints slink's name
        printf("%s\n", suffix);

        counters->slinks++;
    }
    // ======================= FILES =======================
    else if (is_file)
    {
        // Prints file's name
        printf("%s\n", suffix);

        // Gets element's data
        if (!has_data)
        {
            if (lstat(full_path, &st) != 0)
            {
                fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
                counters->error++;
                return;
            }
        }

        counters->files++;
        counters->total_size += st.st_size;
    }
    // ===================== FALLBACK ======================
    else
    {
        counters->others++;
    }

    return;
}
