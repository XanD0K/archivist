#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "cli_parse_common.h"
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
static void report_element(char *current_path, const struct dirent *namelist, ReportOptions *opts,
                           Extension *ext, size_t *ext_counter, size_t *ext_capacity,
                           size_t *total_files, off_t *total_size);
static ssize_t find_extension_in_list(const char *extension, Extension *ext, size_t ext_counter);
static void reallocates_list(Extension **ext, size_t old_capacity);
static void updates_list(Extension *ext, const char *extension_name, size_t ext_counter, size_t index,
                         off_t size, size_t *total_files, off_t *total_size);
static void print_report_output(Extension *ext, size_t ext_counter, bool human_readable,
                                size_t total_files, off_t total_size);
static void clear_new_elements(Extension **ext, size_t old_capacity, size_t new_capacity);

// Creates a report about the content of a given directory
int handle_report(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_report_help,
                                            parse_report_opts, sizeof(ReportOptions));
    if (!context)
    {
        return 10;
    }
    if (context->error_code != 0)
    {
        free_command_context(context);
        return (context->error_code == -1) ? 0 : context->error_code;
    }

    // Parses CLI arguments
    ReportOptions *opts = (ReportOptions*)context->opts;

    // Default sort method
    SortReport sorter = cmp_name_qsort;

    if (opts->base.sort &&  strcasecmp(opts->base.sort, "name") != 0)
    {
        // All sort options
        const char *sorts[] = {"name", "size", "quantity"};
        size_t len = sizeof(sorts) / sizeof(sorts[0]);

        if (!check_sort(opts->base.sort, sorts, len))
        {
            errno = EINVAL;
            fprintf(stderr, "Invalid sort argument: %s\n"
                            "Help for report command: ./archivist report help\n"
                            "Usage: ./archivist report [DIRECTORY] [FLAGS]\n",
                            strerror(errno));
            free_command_context(context);
            return 5;
        }

        sorter = get_sort_func(opts->base.sort);
    }    

    // Parse directory
    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        free_command_context(context);
        return 6;
    }

    // Dynamic Array that holds every extension on directory
    Extension *ext = NULL;
    size_t ext_counter = 0, ext_capacity = 8;
    ext = calloc(ext_capacity, sizeof(Extension));
    if (ext == NULL)
    {
        errno = ENOMEM;
        fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
        free_command_context(context);
        free(namelist);
        return 10;
    }

    // Keeps track of quantity and size of all files
    size_t total_files = 0;
    off_t total_size = 0;

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            report_element(context->base_dir, namelist[i], opts, ext, &ext_counter,
                           &ext_capacity, &total_files, &total_size);
            if (ext == NULL)
            {
                errno = ENOMEM;
                fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
                free_command_context(context);
                free(namelist);
                free_extensions(ext, ext_counter);
                return 10;
            }
        }

        free(namelist[i]);
    }

    // Retrieves user's selected extensions (-e flag)
    size_t user_ext_counter = 0;
    Extension *user_ext = get_all_extensions(opts->filter.extension, &user_ext_counter);    

    if (user_ext == NULL)
    {
        errno = ENOMEM;
        fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
        free(namelist);
        free_extensions(ext, ext_counter);
        free_command_context(context);
        return 10;
    }

    // Populates user's array with data collected from the directory
    for (size_t i = 0; i < user_ext_counter; i++)
    {
        for (size_t j = 0; j < ext_counter; j++)
        {
            if (strcasecmp(user_ext[i].extension, ext[j].extension) == 0)
            {
                user_ext[i].file_count = ext[j].file_count;
                user_ext[i].size = ext[j].size;
            }
        }
    }

    // Sort struct arrays
    Extension *to_print = (user_ext != NULL) ? user_ext : ext;
    size_t print_count = (user_ext != NULL) ? user_ext_counter : ext_counter;
    qsort(to_print, print_count, sizeof(Extension), sorter);

    // Prints output message
    print_report_output(to_print, print_count, opts->base.human_readable, total_files, total_size);

    free_extensions(ext, ext_counter);
    if (user_ext != NULL)
    {
        free_extensions(user_ext, user_ext_counter);
    }
    free(namelist);
    free_command_context(context);
    return 0;
}

// Parses through CLI arguments for 'report' functionality
int parse_report_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    ReportOptions *opts = (ReportOptions*)opts_out;

    int ret;
    ret = parse_common_opts(argc, argv, opt_start, &opts->base);
    if (ret != 0)
    {
        return ret;
    }
    ret = parse_filter_options(argc, argv, opt_start, &opts->filter);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
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

// Returns specific sort function based on chosen sort method
static SortReport get_sort_func(char *sort)
{
    if (strcasecmp(sort, "name") == 0)
    {
        return cmp_name_qsort;
    }
    else if (strcasecmp(sort, "size") == 0)
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

// Organizes by name
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

// Organizes by size
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

// Organizes by quantity
static int cmp_quantity_qsort(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    return ((int)extB->file_count - (int)extA->file_count);
}

// Calculates porpotion of each extension on given directory
static void report_element(char *current_path, const struct dirent *namelist, ReportOptions *opts,
                           Extension *ext, size_t *ext_counter, size_t *ext_capacity,
                           size_t *total_files, off_t *total_size)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    char new_path[PATH_MAX];
    if (check_path_name_size(new_path, sizeof(new_path), current_path, namelist->d_name) == -1)
    {
        return;
    }

    struct stat st;
    if (stat(new_path, &st) != 0)
    {
        return;
    }

    // Checks for recursive flag
    if (opts->base.recursive && S_ISDIR(st.st_mode))
    {
        struct dirent **entry;
        int n = scandir(new_path, &entry, NULL, alphasort);
        if (n == -1)
        {
            return;
        }
        for (int i = 0; i < n; i++)
        {
            report_element(new_path, entry[i], opts, ext, ext_counter,
                           ext_capacity, total_files, total_size);
            free(entry[i]);
        }

        free(entry);
    }

    // Ignores directories
    if (S_ISDIR(st.st_mode))
    {
        return;
    }

    // Gets extension for current element
    const char *extension_name = get_clean_extension(namelist->d_name);
    // Checks if extension already exists
    ssize_t index = find_extension_in_list(extension_name, ext, *ext_counter);
    // Current extension doesn't exist in list
    if (index == -1)
    {
        // Checks necessity of realloc (factor 0.75)
        float factor = (float)*ext_counter / (float)*ext_capacity;        
        if (factor >= 0.75f)
        {
            // Reallocates memory for new entry
            reallocates_list(&ext, *ext_capacity);
            if (ext == NULL)
            {
                return;
            }
            // Doubles list's capacity
            *ext_capacity *= 2;
        }
        // Adds new extension to list
        updates_list(ext, extension_name, *ext_counter, *ext_counter,
                     st.st_size, total_files, total_size);
        // Increments extension's counter
        (*ext_counter)++;
    }
    else
    {
        updates_list(ext, extension_name, *ext_counter, (size_t)index,
                     st.st_size, total_files, total_size);
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
static void updates_list(Extension *ext, const char *extension_name, size_t ext_counter, size_t index,
                         off_t size, size_t *total_files, off_t *total_size)
{
    // Adding new entry
    if (index == ext_counter)
    {
        ext[index].extension = (strcmp(extension_name, "") == 0)
            ? strdup("others")
            : strdup(extension_name);
        ext[index].file_count = 0;
        ext[index].size = 0;
    }
    // Updating existed entry
    ext[index].file_count++;
    ext[index].size += size;

    (*total_files)++;
    (*total_size) += size;
}

// Prints output message with percentage for each extension
static void print_report_output(Extension *ext, size_t ext_counter, bool human_readable,
                                size_t total_files, off_t total_size)
{
    const float percentage = 100.0f;
    for (size_t i = 0; i < ext_counter; i++)
    {
        float file_percentage = (total_files > 0)
            ? (float)ext[i].file_count / (float)total_files * percentage
            : 0.0f;
        float size_percentage = (total_size > 0)
            ? (float)ext[i].size / (float)total_size * percentage
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
