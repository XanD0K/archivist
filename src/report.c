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
#include "report.h"
#include "utils.h"

// Prototypes
static void print_report_help(void);
static ReportOptions parse_report_opts(int argc, char **argv, int opt_start);
static SortReport get_sort_func(char *sort);
static int cmp_name(const void *a, const void *b);
static int cmp_size(const void *a, const void *b);
static int cmp_quantity(const void *a, const void *b);
static void report_element(char *current_path, const struct dirent *namelist, ReportOptions opts,
                           Extension *ext, size_t *ext_counter, size_t *ext_capacity,
                           size_t *total_files, off_t *total_size);
static ssize_t find_extension_in_list(const char *extension, Extension *ext, size_t ext_counter);
static void reallocates_list(Extension **ext, size_t old_capacity);
static void updates_list(Extension *ext, const char *extension_name, size_t ext_counter, size_t index,
                         off_t size, size_t *total_files, off_t *total_size);
static void print_report_output(Extension *ext, size_t ext_counter, bool human_readable,
                                size_t total_files, off_t total_size);
static void clear_new_elements(Extension **ext, size_t old_capacity, size_t new_capacity);
static void free_array(Extension *ext, size_t ext_capacity);

// Creates a report about the content of a given directory
int handle_report(int argc, char **argv)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[2]))
    {
        // Prints help message for 'report' functionality
        print_report_help();
        return 0;
    }

    // Defines starting values
    char *dir_path = NULL;
    int opt_start = 2;
    if (argc >= 3 && argv[2][0] != '-')
    {
        dir_path = argv[2];
        opt_start = 3;
    }

    // Gets valid base directory (default: .)
    char *base_dir = get_valid_directory(dir_path);
    if (!base_dir)
    {
        return 4;
    }

    // Parses CLI arguments
    ReportOptions opts = parse_report_opts(argc, argv, opt_start);

    // Default sort method
    SortReport sorter = cmp_name;

    if (opts.base.sort != NULL &&  strcasecmp(opts.base.sort, "name") != 0)
    {
        // All sort options
        const char *sorts[] = {"name", "size", "quantity"};
        size_t len = sizeof(sorts) / sizeof(sorts[0]);

        if (!check_sort(opts.base.sort, sorts, len))
        {
            errno = EINVAL;
            fprintf(stderr, "Invalid sort argument: %s\n"
                            "Help for report command: ./archivist report help\n"
                            "Usage: ./archivist report [DIRECTORY] [FLAGS]\n",
                            strerror(errno));
            free(base_dir);
            return 5;
        }

        sorter = get_sort_func(opts.base.sort);
    }    

    // Parse directory
    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        free(base_dir);
        perror("scandir");
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
        free(base_dir);
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
            report_element(base_dir, namelist[i], opts, ext, &ext_counter,
                           &ext_capacity, &total_files, &total_size);
            free(namelist[i]);
            if (ext == NULL)
            {
                errno = ENOMEM;
                fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
                free(base_dir);
                free(namelist);
                return 10;
            }
        }
    }

    free(base_dir);

    // Retrieves user's selected extensions (-e flag)
    Extension *user_ext = NULL;
    size_t user_ext_counter = 0;
    if (opts.base.extension != NULL)
    {
        char *token = strtok(opts.base.extension, ",");
        while (token != NULL)
        {
            user_ext = realloc(user_ext, (user_ext_counter + 1) * sizeof(Extension));
            if (user_ext == NULL)
            {
                errno = ENOMEM;
                fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
                free(namelist);
                return 10;
            }
            user_ext[user_ext_counter].extension = strdup(token);
            user_ext[user_ext_counter].file_count = 0;
            user_ext[user_ext_counter].size = 0;
            user_ext_counter++;
            token = strtok(NULL, ",");
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
    }

    // Sort struct arrays
    Extension *to_print = (user_ext != NULL) ? user_ext : ext;
    size_t print_count = (user_ext != NULL) ? user_ext_counter : ext_counter;
    qsort(to_print, print_count, sizeof(Extension), sorter);

    // Prints output message
    print_report_output(to_print, print_count, opts.base.human_readable, total_files, total_size);

    free_array(ext, ext_counter);
    if (user_ext != NULL)
    {
        free_array(user_ext, user_ext_counter);
    }
    free(namelist);
    return 0;
}

// Prints explanation of 'report' functionality
static void print_report_help(void)
{
    puts(
        "Usage: ./archivist report [DIRECTORY] [FLAGS]\n"
        "\n"
        "DIRECTORY defaults to current directory (.)\n"
        "\n"
        "Flags:\n"
        "   -e | --extension\n"
        "       displays information only of specified extensions\n"
        "       if more than 1 extension, separate them with a comma\n"
        "   -h | --human-readable\n"
        "       outputs size in a more readable format\n"
        "       default: off\n"
        "   -s | --sort <name | size | quantity>\n"
        "       sorts output by:\n"
        "           name → compares the ASCII value of each character\n"
        "           size → total size\n"
        "           quantity → number of files\n"
        "       default: name"
        "   -R | --recursive\n"
        "       also lists subdirectories\n"
        "       default: off\n"
        "\n"
        "Examples:\n"
        "   ./archivist report\n"
        "   ./archivist report /folder\n"
        "   ./archivist report /folder -e txt,pdf,jpeg\n"
        "   ./archivist report --human-readable -R\n"
        "\n"
        "All commands: ./archivist help"
    );
}

// Parses through CLI arguments for 'report' functionality
static ReportOptions parse_report_opts(int argc, char **argv, int opt_start)
{
    ReportOptions opts = {0};

    static struct option long_opts[] =
    {
        {"extension", required_argument, 0, 'e'},
        {"human-readable", no_argument, 0, 'h'},
        {"sort", required_argument, 0, 's'},
        {"recursive", no_argument, 0, 'R'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "e:hs:R";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'e':
            {
                opts.base.extension = optarg;
                break;
            }
            case 'h':
            {
                opts.base.human_readable = true;
                break;
            }
            case 's':
            {
                opts.base.sort = optarg;
                break;
            }
            case 'R':
            {
                opts.base.recursive = true;
                break;
            }
            case 0:
            case '?':
            {
                break;
            }
        }
    }

    return opts;
}

// Returns specific sort function based on chosen sort method
static SortReport get_sort_func(char *sort)
{
    if (strcasecmp(sort, "name") == 0)
    {
        return cmp_name;
    }
    else if (strcasecmp(sort, "size") == 0)
    {
        return cmp_size;
    }
    else if (strcasecmp(sort, "quantity") == 0)
    {
        return cmp_quantity;
    }

    // Fallback
    return cmp_name;
    
}

// Organizes by name
static int cmp_name(const void *a, const void *b)
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
static int cmp_size(const void *a, const void *b)
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
static int cmp_quantity(const void *a, const void *b)
{
    const Extension *extA = (const Extension *)(a);
    const Extension *extB = (const Extension *)(b);

    return ((int)extB->file_count - (int)extA->file_count);
}

// Calculates porpotion of each extension on directory
static void report_element(char *current_path, const struct dirent *namelist, ReportOptions opts,
                           Extension *ext, size_t *ext_counter, size_t *ext_capacity,
                           size_t *total_files, off_t *total_size)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    struct stat st;
    char new_path[PATH_MAX];

    snprintf(new_path, sizeof(new_path), "%s/%s", current_path, namelist->d_name);
    if (stat(new_path, &st) != 0)
    {
        return;
    }

    // Checks for recursive flag
    if (opts.base.recursive && S_ISDIR(st.st_mode))
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
    const char *extension_name = get_extension(namelist->d_name);
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

// Frees array of Extension structures, and each of their 'extension' field
static void free_array(Extension *ext, size_t ext_capacity)
{
    if (ext == NULL)
    {
        return;
    }
    for (size_t i = 0; i < ext_capacity; i++)
    {
        free(ext[i].extension);
    }
    free(ext);
}
