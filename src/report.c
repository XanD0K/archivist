#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <errno.h>
#include <getopt.h>
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
#include "report.h"
#include "utils.h"
#include "utils_sort.h"

// Prototypes
static void report_element(const struct dirent *namelist, ReportOptions *opts,
                           Extension **ext, const char *current_path,
                           ReportCounters *counters, ScandirFilter filter);
static ssize_t find_extension_in_list(const char *extension, Extension *ext, size_t ext_counter);
static void reallocates_list(Extension **ext, size_t *capacity);
static void updates_list(Extension *ext, const char *extension_name, size_t index,
                         off_t file_size, ReportCounters *counters);
static void print_report_output(Extension *ext, size_t ext_counter,
                                bool human_readable, ReportCounters counters);

// Setup logic for 'report' feature
ErrorCode handle_report(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_report_help,
                                            parse_report_opts, sizeof(ReportOptions));
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

    ReportOptions *opts = (ReportOptions*)context->opts;

    // Initializes counters
    ReportCounters counters = {0};
    counters.ext_capacity = 8;  // Initial value for Dynamic Array of 'Extension'

    // Retrieves user's selected extensions (-e|--extension)
    Extension *ext = NULL;
    ext = calloc(counters.ext_capacity, sizeof(Extension));
    if (!ext)
    {
        fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
        free_command_context(context);
        return EC_MEMORY_ALLOCATION;
    }
    
    // Determines the filter used in scandir()
    ScandirFilter filter = (opts->base.all)
        ? scandir_no_dot_filter
        : scandir_visible_only;

    // Default return value
    ErrorCode ret = EC_SUCCESS;

    // Initializes variables
    Extension *user_ext = NULL;

    // Retrieves directory's content
    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        ret = EC_SCANDIR_ERROR;
        goto cleanup;
    }

    for (int i = 0; i < n; i++)
    {
        report_element(namelist[i], opts, &ext, context->base_dir, &counters, filter);
        // Prevents code from continue running with a corrupted Dynamic Array
        if (!ext)
        {
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            ret = EC_MEMORY_ALLOCATION;
            goto cleanup;
        }
    }

    // Retrieves user's selected extensions (-e|--extension flag)
    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        user_ext = get_all_extensions(opts->filter.extension, &counters.user_ext);
        if (!user_ext)
        {
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            ret = EC_MEMORY_ALLOCATION;
            goto cleanup;
        }
    }

    // Populates user's array with data collected from the directory
    for (size_t i = 0; i < counters.user_ext; i++)
    {
        for (size_t j = 0; j < counters.ext; j++)
        {
            if (strcasecmp(user_ext[i].extension, ext[j].extension) == 0)
            {
                user_ext[i].file_count = ext[j].file_count;
                user_ext[i].size = ext[j].size;
            }
        }
    }

    // Default sorter function (by name)
    SortQsort sorter = cmp_name_qsort;
    // Checks for 'sort' flag
    if (opts->base.sort && opts->base.sort[0] != '\0' && 
        strcasecmp(opts->base.sort, "name") != 0)
    {
        // All sort methods available
        const char *sorts[] = {"size", "quantity"};
        size_t len = sizeof(sorts) / sizeof(sorts[0]);

        // Updates sorter function
        sorter = get_qsort_sort_fn(opts->base.sort, sorts, len);
        if (!sorter)
        {
            ret = EC_INVALID_SORTER;
            goto cleanup;
        }
    }

    // Sorts struct array
    Extension *to_print = (user_ext) ? user_ext : ext;
    size_t print_count = (user_ext) ? counters.user_ext : counters.ext;
    qsort(to_print, print_count, sizeof(Extension), sorter);

    // Prints output message
    print_report_output(to_print, print_count, opts->base.human_readable, counters);
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
    if (user_ext)
    {
        free_extensions(user_ext, counters.user_ext);
    }

    free_command_context(context);
    return ret;
}

// Parses through CLI arguments for 'report' functionality
ErrorCode parse_report_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    ReportOptions *opts = (ReportOptions*)opts_out;
    opts->base.sort = "name";

    opterr = 0;

    // Supported flags
    uint32_t common_flags = COMMON_ALL |
                            COMMON_HUMAN_READABLE |
                            COMMON_RECURSIVE |
                            COMMON_SORT;
    uint32_t filter_flags = FILTER_EXTENSION;

    static struct option long_opts[] =
    {
        // Common flags
        {"all", no_argument, 0, 'a'},
        {"human-readable", no_argument, 0, 'h'},
        {"sort", required_argument, 0, 's'},
        {"recursive", no_argument, 0, 'R'},
        // Filter flags
        {"extension", required_argument, 0, 'e'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "ahs:Re:";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // ========== COMMON ==========
            case 'a':  // all
            case 'h':  // human-readable
            case 's':  // sort
            case 'R':  // recursive
            {
                handle_common_flag(opt, optarg, &opts->base, common_flags);
                break;
            }
            // ========== FILTER ==========
            case 'e':  // extension
            {
                handle_filter_flag(opt, long_opts[long_index].name, optarg, &opts->filter, filter_flags);
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

    return EC_SUCCESS;
}

// Calculates porpotion of each extension on given directory
static void report_element(const struct dirent *namelist, ReportOptions *opts,
                           Extension **ext, const char *current_path,
                           ReportCounters *counters, ScandirFilter filter)
{
    // Builds full path to current element
    char full_path[PATH_MAX];
    if (check_path_name_size(full_path, sizeof(full_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
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

    // ==================== Directories ====================
    if (is_dir)
    {
        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
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
                report_element(entry[i], opts, ext, full_path, counters, filter);
                if (*ext == NULL)
                {
                    free_dirent(entry, n);
                    return;
                }
            }

            free_dirent(entry, n);
        }

        return;
    }

    // ======================= FILES =======================
    // Gets extension for current element
    const char *extension_name = get_clean_extension(namelist->d_name);
    // Checks if extension already exists
    ssize_t index = find_extension_in_list(extension_name, *ext, counters->ext);

    // Gets element's data
    struct stat st;
    if (lstat(full_path, &st) != 0)
    {
        fprintf(stderr, "Couldn't access %s: %s\n", full_path, strerror(errno));
        counters->error++;
        return;
    }

    // Current extension doesn't exist in list
    if (index == -1)
    {
        // Checks necessity of realloc (factor 0.75)
        if ((float)counters->ext / (float)counters->ext_capacity >= 0.75f)
        {
            // Reallocates memory for new entry
            reallocates_list(ext, &counters->ext_capacity);
            if (*ext == NULL)
            {
                fprintf(stderr, "Error on realloc(): %s\n", strerror(errno));
                counters->error++;
                return;
            }
        }
        // Adds new extension to list
        updates_list(*ext, extension_name, counters->ext, st.st_size, counters);
        // Increments extension's counter
        counters->ext++;
    }
    // Extension already exists in list (updates values)
    else
    {
        updates_list(*ext, extension_name, (size_t)index, st.st_size, counters);
    }
}

// Searches for extension in extension list
static ssize_t find_extension_in_list(const char *extension, Extension *ext, size_t ext_counter)
{
    for (ssize_t i = 0; i < (ssize_t)ext_counter; i++)
    {
        if ((strcasecmp(extension, ext[i].extension) == 0) ||
            (strcmp(extension, "") == 0 && strcasecmp(ext[i].extension, "others") == 0))
        {
            return i;
        }
    }

    return -1;
}

// Doubles memory to prevent overflow
static void reallocates_list(Extension **ext, size_t *capacity)
{
    size_t old_capacity = *capacity;
    size_t new_capacity = 2 * old_capacity;
    Extension *new_ext = realloc(*ext, new_capacity * sizeof(Extension));
    if (!new_ext)
    {
        *ext = NULL;
        return;
    }

    // Clears new allocated memory
    memset(new_ext + old_capacity, 0, (new_capacity - old_capacity) * sizeof(Extension));

    *ext = new_ext;
    *capacity = new_capacity;
}

// Updates list of extensions
static void updates_list(Extension *ext, const char *extension_name, size_t index,
                         off_t file_size, ReportCounters *counters)
{
    // Adding new entry
    if (index == counters->ext)
    {
        ext[index].extension = (strcmp(extension_name, "") == 0)
            ? strdup("others")
            : strdup(extension_name);
        ext[index].file_count = 0;
        ext[index].size = 0;
    }
    // Updating existed entry
    ext[index].file_count++;
    ext[index].size += file_size;

    counters->total_files++;
    counters->total_size += file_size;
}

// Prints output message with percentage for each extension
static void print_report_output(Extension *ext, size_t ext_counter, bool human_readable, ReportCounters counters)
{
    size_t quantity_total = 0;
    off_t size_total = 0;
    const float percentage = 100.0f;
    for (size_t i = 0; i < ext_counter; i++)
    {
        float file_percentage = (counters.total_files > 0)
            ? (float)ext[i].file_count / (float)counters.total_files * percentage
            : 0.0f;
        float size_percentage = (counters.total_size > 0)
            ? (float)ext[i].size / (float)counters.total_size * percentage
            : 0.0f;
        quantity_total += ext[i].file_count;
        size_total += ext[i].size;
        if (human_readable)
        {
            char *str = formatted_output(ext[i].size);
            printf("%-9s  |  %5zu files (%6.2f%%)  |  %12s (%6.2f%%)\n",
                   ext[i].extension, ext[i].file_count, file_percentage,
                   str, size_percentage);
            free(str);
        }
        else
        {
            printf("%-9s  |  %5zu files (%6.2f%%)  |  %12jd bytes (%6.2f%%)\n",
                   ext[i].extension, ext[i].file_count, file_percentage,
                   (__intmax_t)ext[i].size, size_percentage);
        }
    }

    printf("---------------------------------------------------------------------\n");
    if (human_readable)
    {
        printf("Total      |  %5zu files (100.00%%)  |  %12ld bytes (100.00%%)\n",
               quantity_total, size_total);
    }
    else
    {
        printf("Total      |  %5zu files (100.00%%)  |  %12jd bytes (100.00%%)\n",
               quantity_total, (__intmax_t)size_total);
    }
}
