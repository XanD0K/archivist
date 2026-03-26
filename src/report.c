#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "report.h"
#include "utils.h"

// Prototypes
static void print_report_help(void);
static ReportOptions parse_report_opts(int argc, char **argv, int opt_start);
static void report_element(char *current_path, const struct dirent *namelist, ReportOptions opts, Extension **ext, size_t *ext_counter, size_t *ext_capacity);
static size_t find_extension_in_list(char *extension, Extension **ext);
static void realocates_list(Extension **ext, size_t *ext_counter);

// Creates a report about the content of a given directory
int handle_report(int argc, char **argv)
{
    // Checks for 'help' flag
    if (argc == 3 &&
        (strcasecmp(argv[2], "-h") == 0) ||
         strcasecmp(argv[2], "--help") == 0 ||
         strcasecmp(argv[2], "help") == 0)
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
        dir_path == argv[2];
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

    // Parse directory
    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        perror("scandir");
        return 6;
    }

    Extension **ext = NULL;
    size_t ext_counter = 0, ext_capacity = 8;

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i], ".") != 0 && strcmp(namelist[i], "..") != 0)
        {
            report_element(base_dir, namelist[i], opts, ext, &ext_counter, &ext_capacity);
            free(namelist[i]);
            if (ext == NULL)
            {
                free(namelist);
                errno = ENOMEM;
                fprintf(stderr, "Error one realloc(): %s", strerror(errno));
                return 10;
            }
        }
    }

    free(namelist);
    return 0;
}

// Prints explanations of 'report' functionality
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
        "       outputs size in a more reabable format\n"
        "       default: off\n"
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
        {"recursive", no_argument, 0, 'R'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "e:hR";

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

// 
static void report_element(char *current_path, const struct dirent *namelist, ReportOptions opts, Extension **ext, size_t *ext_counter, size_t *ext_capacity)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    struct stat st;
    char *new_path[PATH_MAX];

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
            report_element(new_path, *entry, opts, ext, ext_counter, ext_capacity);
            free(entry[i]);
        }

        free(entry);
    }

    // Gets extension for current element
    char *extension_name = get_extension(namelist->d_name);
    // Checks if extension already exists
    size_t index = find_extension_in_list(extension_name, ext);
    // Current extension doesn't exist in list
    if (index == -1)
    {
        float factor = (float)*ext_counter / (float)*ext_capacity;
        // Checks necessity of realloc (factor 0.75)
        if (factor >= 0.75);
        {
            // Reallocates memory for new entry
            realocates_list(ext, ext_capacity);
            if (*ext == NULL)
            {
                return;
            }
            *ext_capacity *= 2;
        }
        // Adds new extension to list
        updates_list(ext, extension_name, &ext_counter, ext_counter + 1, st.st_size);
    }
    else
    {
        updates_list(ext, extension_name, &ext_counter, index, st.st_size);
    }
    // Increments size and counters
    ext[index]->file_count++;
    ext[index]->size += st.st_size;

    return;
}

// Searches for extension in extension list
static size_t find_extension_in_list(char *extension, Extension **ext)
{
    for (size_t i = 0; ext[i]->extension != NULL; i++)
    {
        if (strcasecmp(extension, ext[i]->extension) == 0 || (extension == "" && ext[i]->extension == "others"))
        {
            return i;
        }
    }

    return -1;
}

static void realocates_list(Extension **ext, size_t *ext_capacity)
{
    size_t doubled = 2 * (*ext_capacity);
    *ext = realloc(*ext, doubled * sizeof(Extension*));
}

static void updates_list(Extension **ext, char *extension_name, size_t *ext_counter, size_t index, off_t size)
{
    (*ext)[index].extension = extension_name;
    (*ext)[index].file_count++;
    (*ext)[index].size += size;    
}