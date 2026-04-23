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

// Prototypes
static bool check_sort(char *sort, const char **sorts, size_t len);
static SortReport get_sort_func(char *sort);
static int cmp_name_qsort(const void *a, const void *b);
static int cmp_size_qsort(const void *a, const void *b);
static int cmp_quantity_qsort(const void *a, const void *b);
static void report_element(const char *current_path, const struct dirent *namelist, ReportOptions *opts,
                           Extension **ext, ReportCounters *counters);
static ssize_t find_extension_in_list(const char *extension, Extension *ext, size_t ext_counter);
static void reallocates_list(Extension **ext, size_t old_capacity);
static void updates_list(Extension *ext, const char *extension_name, size_t index,
                         off_t file_size, ReportCounters *counters);
static void print_report_output(Extension *ext, size_t ext_counter,
                                bool human_readable, ReportCounters counters);
static void clear_new_elements(Extension **ext, size_t old_capacity, size_t new_capacity);

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

    // Default sorter function (by name)
    SortReport sorter = cmp_name_qsort;
    if (opts->base.sort && strcasecmp(opts->base.sort, "name") != 0)
    {
        // All sort methods available
        const char *sorts[] = {"size", "quantity"};
        size_t len = sizeof(sorts) / sizeof(sorts[0]);

        // Checks for valid sorter method
        if (!check_sort(opts->base.sort, sorts, len))
        {
            errno = EINVAL;
            fprintf(stderr, "Invalid sort argument: %s\n", strerror(errno));
            free_command_context(context);
            return EC_INVALID_SORTER;
        }

        // Updates sorter function
        sorter = get_sort_func(opts->base.sort);
    }

    // Parse directory
    struct dirent **namelist = NULL;
    int n = scandir(context->base_dir, &namelist, scandir_no_dot_filter, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        free_command_context(context);
        return EC_SCANDIR_ERROR;
    }

    // Initializes counters
    ReportCounters counters = {0};
    counters.ext_capacity = 8;

    // Default return value
    ErrorCode ret = EC_SUCCESS;

    // Initializes variables
    Extension *user_ext = NULL;


    // Dynamic Array that holds every extension on directory
    Extension *ext = NULL;
    ext = calloc(counters.ext_capacity, sizeof(Extension));
    if (ext == NULL)
    {
        fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
        ret = EC_MEMORY_ALLOCATION;
        goto cleanup;
    }

    for (int i = 0; i < n; i++)
    {
        report_element(context->base_dir, namelist[i], opts, &ext, &counters);
        if (ext == NULL)
        {
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            ret = EC_MEMORY_ALLOCATION;
            goto cleanup;
        }
    }
    free_dirent(namelist, n);

    // Retrieves user's selected extensions (-e|--extension flag)
    user_ext = get_all_extensions(opts->filter.extension, &counters.user_ext);
    if (user_ext == NULL)
    {
        fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
        ret = EC_MEMORY_ALLOCATION;
        goto cleanup;
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

    // Sorts struct arrays
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

    uint32_t supported_flags = COMMON_HUMAN_READABLE |
                               COMMON_RECURSIVE |
                               COMMON_SORT |
                               FILTER_EXTENSION;

    ErrorCode ret;
    ret = parse_common_opts(argc, argv, opt_start, &opts->base, supported_flags);
    if (ret != EC_SUCCESS)
    {
        return ret;
    }
    ret = parse_filter_options(argc, argv, opt_start, &opts->filter, supported_flags);
    if (ret != EC_SUCCESS)
    {
        return ret;
    }

    return EC_SUCCESS;
}

// Checks for valid sort method
static bool check_sort(char *sort, const char **sorts, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (strcasecmp(sort, sorts[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

// Returns specific sort function based on chosen method
static SortReport get_sort_func(char *sort)
{
    if (strcasecmp(sort, "size") == 0)
    {
        return cmp_size_qsort;
    }
    else if (strcasecmp(sort, "quantity") == 0)
    {
        return cmp_quantity_qsort;
    }

    // Fallback
    return cmp_name_qsort;
}

// Sorts by name
static int cmp_name_qsort(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    if (strcasecmp(extA->extension, "others") == 0)
    {
        return 1;
    }
    if (strcasecmp(extB->extension, "others") == 0)
    {
        return -1;
    }

    return (strcasecmp(extA->extension, extB->extension));
}

// Sorts by size
static int cmp_size_qsort(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    if (extA->size > extB->size) 
    {
        return 1;
    }
    else if (extA->size < extB->size)
    {
        return -1;
    }
    return 0;
}

// Sorts by quantity
static int cmp_quantity_qsort(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    return ((int)extB->file_count - (int)extA->file_count);
}

// Calculates porpotion of each extension on given directory
static void report_element(const char *current_path, const struct dirent *namelist, ReportOptions *opts,
                           Extension **ext, ReportCounters *counters)
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

    // ==================== Directories ====================
    if (is_dir)
    {
        // Recursively traverses subdirectories
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(full_path, &entry, scandir_no_dot_filter, alphasort);
            if (n == -1)
            {
                fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
                counters->error++;
                return;
            }
            for (int i = 0; i < n; i++)
            {
                report_element(full_path, entry[i], opts, ext, counters);
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

    // ======================= Files =======================
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
        float factor = (float)counters->ext / (float)counters->ext_capacity;        
        if (factor >= 0.75f)
        {
            // Reallocates memory for new entry
            reallocates_list(ext, counters->ext_capacity);
            if (*ext == NULL)
            {
                fprintf(stderr, "Error on realloc(): %s\n", strerror(errno));
                counters->error++;
                return;
            }
            // Doubles list's capacity
            counters->ext_capacity *= 2;
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

    return;
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
static void reallocates_list(Extension **ext, size_t old_capacity)
{
    size_t doubled = 2 * old_capacity;
    Extension *new_ext = realloc(*ext, doubled * sizeof(Extension));
    if (new_ext == NULL)
    {
        *ext = NULL;
        return;
    }

    // Clears new allocated memory
    clear_new_elements(&new_ext, old_capacity, doubled);
    *ext = new_ext;
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
    const float percentage = 100.0f;
    for (size_t i = 0; i < ext_counter; i++)
    {
        float file_percentage = (counters.total_files > 0)
            ? (float)ext[i].file_count / (float)counters.total_files * percentage
            : 0.0f;
        float size_percentage = (counters.total_size > 0)
            ? (float)ext[i].size / (float)counters.total_size * percentage
            : 0.0f;
        if (human_readable)
        {
            char *str = formatted_output(ext[i].size);
            printf("%s  |  %zu files (%.2f%%)  |  %s (%.2f%%)\n",
                   ext[i].extension, ext[i].file_count, file_percentage,
                   str, size_percentage);
            free(str);
        }
        else
        {
            printf("%s  |  %zu files (%.2f%%)  |  %jd bytes (%.2f%%)\n",
                   ext[i].extension, ext[i].file_count, file_percentage,
                   (__intmax_t)ext[i].size, size_percentage);
        }
    }
}

// Clears fields for new allocated memory
static void clear_new_elements(Extension **ext, size_t old_capacity, size_t new_capacity)
{
    for (size_t i = old_capacity; i < new_capacity; i++)
    {
        (*ext)[i].extension = NULL;
        (*ext)[i].file_count = 0;
        (*ext)[i].size = 0;
    }
}

