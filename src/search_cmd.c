#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "search_cmd.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void print_search_help(void);
static SearchOptions parse_search_opts(int argc, char **argv, int opt_start, bool *size_err);
static void search_element(char *current_path, const char *base_dir, SearchOptions opts, const struct dirent *namelist, const char *searched, size_t *counter, bool *printed);
static bool match_name(const char *current_name, const char *searched, bool contains, bool ignore_case);
static bool match_extension(const char *current_name, const char *ext);

// Searches for a specific file/directory
int handle_search(int argc, char **argv)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[2]))
    {
        // Prints help message for 'search' functionality
        print_search_help();
        return 0;
    }

    char *searched_name = strdup(argv[2]);
    if (!searched_name)
    {
        fprintf(stderr, "Couldn't duplicate string '%s': %s\n", argv[2], strerror(errno));
        return 8;
    }

    // Defines starting values
    const char *dir_path = NULL;
    int opt_start = 3;
    // Directory was provided
    if (argc > 3 && argv[3][0] != '-')
    {
        dir_path = argv[3];
        opt_start = 4;
    }

    // Gets valid directory (default: .)
    char *base_dir = get_valid_directory(dir_path);
    if (!base_dir)
    {
        return 4;
    }

    bool size_err = false;
    // Parses CLI arguments
    SearchOptions opts = parse_search_opts(argc, argv, opt_start, &size_err);
    if (size_err)
    {
        free(base_dir);
        errno = EIO;
        fprintf(stderr, "Invalid size: %s", strerror(errno));
        return 9;
    }

    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, alphasort);

    if (n == -1)
    {
        free(base_dir);
        perror("scandir");
        return 6;
    }

    size_t counter = 0;
    bool printed = false;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            search_element(base_dir, base_dir, opts, namelist[i], searched_name, &counter, &printed);
            free(namelist[i]);
        }
    }
    if (!printed)
    {
        if (!opts.base.recursive)
        {
            printf("Couldn't find '%s' on '%s' base directory", searched_name, base_dir);
        }
        else
        {
            printf("Couldn't find '%s' on base directory '%s' and in any other of its subdirectory", searched_name, base_dir);
        }        
    }

    free(namelist);
    free(base_dir);
    free(searched_name);
    return 0;
}

// Prints explanation of 'search' functionality
static void print_search_help(void)
{
    puts(
        "Usage: ./archivist search NAME [DIRECTORY] [FLAGS]\n"
        "\n"
        "DIRECTORY defaults to current directory (.)\n"
        "If NAME is not important, use * symbol"
        "\n"
        "Flags:\n"
        "   -c | --contains\n"
        "       includes all files/directories that look alike given name\n"
        "       default: off\n"
        "   -e | --extension\n"
        "       search for specific extension\n"
        "       e.g. txt | .txt"
        "   -i | --ignore-case\n"
        "       distinguish case\n"
        "       default: on\n"
        "   -t | --type\n"
        "       includes only specific type (file | dir | slink)\n"
        "   -R | --recursive\n"
        "       also search in subdirectories\n"
        "       default: off\n"
        "   --min-size\n"
        "   --max-size\n"
        "       only consider files smaller/larger than specified value\n"
        "       notation: B | K | KB | M | MB | G | GB | T | TB\n"
        "       default: B (bytes)\n"
        "       e.g. 20 | 50K | 30GB | 200T\n"
        "\n"
        "Examples:\n"        
        "./archivist search filename.txt\n"
        "./archivist search filename.txt /folder\n"
        "./archivist search filename /folder -e .txt \n"
        "./archivist search * /folder -e txt \n"
        "./archivist search filen /folder -c --ignore-case -t file\n"
        "./archivist search filen /folder -R --min-size 50K --max-size 50G\n"
        "\n"
        "All comands: ./archivist help"
    );
}

// Parses through CLI arguments for 'search' functionality
static SearchOptions parse_search_opts(int argc, char **argv, int opt_start, bool *size_err)
{
    SearchOptions opts = {0};

    // Set default values
    opts.base.ignore_case = true;

    static struct option long_opts[] =
    {
        {"contains", no_argument, 0, 'c'},
        {"extension", required_argument, 0, 'e'},
        {"ignore-case", no_argument, 0, 'i'},
        {"type", required_argument, 0, 't'},
        {"recursive", no_argument, 0, 'R'},
        {"max-size", required_argument, 0, 0},
        {"min-size", required_argument, 0, 0},
        
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "ce:it:R";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'R':
            {
                opts.base.recursive = true;
                break;
            }
            case 'i':
            {
                opts.base.ignore_case = false;
                break;
            }
            case 'c':
            {
                opts.contains = true;
                break;
            }
            case 't':
            {
                opts.filter.type = optarg;
                break;
            }
            case 'e':
            {
                opts.base.extension = optarg;
                break;
            }
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "min-size") == 0)
                {
                    opts.filter.min_size = get_size(optarg);
                }
                else if (strcmp(long_opts[long_index].name, "max-size") == 0)
                {
                    opts.filter.max_size = get_size(optarg);
                }
                if (opts.filter.max_size == -1 || opts.filter.min_size == -1)
                {
                    *size_err = true;
                }
                break;
            }
            case '?':
            {
                break;
            }
        }
    }

    return opts;
}

// Searches for an element
static void search_element(char *current_path, const char *base_dir, SearchOptions opts, const struct dirent *namelist, const char *searched, size_t *counter, bool *printed)
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

    // Checks recursively for searched element
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
            search_element(new_path, base_dir, opts, entry[i], searched, counter, printed);
            free(entry[i]);
        }

        free(entry);
    }

    // Checks for name equality
    if (strcmp(namelist->d_name, "*") != 0 && !match_name(namelist->d_name, searched, opts.contains, opts.base.ignore_case))
    {
        return;
    }

    // Checks for extension
    if (opts.base.extension && !match_extension(opts.base.extension, namelist->d_name))
    {
        return;
    }

    // Checks for element's type
    if (opts.filter.type && !match_type(opts.filter.type, st.st_mode))
    {
        return;
    }

    // Checks for size
    if ((opts.filter.max_size || opts.filter.min_size) && !match_size(opts.filter.max_size, opts.filter.min_size, st.st_size))
    {
        return;
    }

    // Prints found element
    const char *suffix = new_path + strlen(base_dir);
    if (*suffix == '/')
    {
        suffix++;
    }
    printf("%s\n", suffix);
    (*counter)++;
    *printed = true;
    
    return;
}

// Checks if strings match
static bool match_name(const char *current_name, const char *searched, bool contains, bool ignore_case)
{
    // Exact match
    if(!contains)
    {
        return (ignore_case) ? (strcasecmp(current_name, searched) == 0) : (strcmp(current_name, searched) == 0);
    }
    // Partial match (Contains = True)
    char *result = (ignore_case) ? (strcasestr(current_name, searched)) : (strstr(current_name, searched));
    return result != NULL;
}

static bool match_extension(const char *current_name, const char *ext)
{
    const char *ext_name = get_extension(current_name);
    const char *clean_ext = (ext[0] == '.') ? ext + 1 : ext;

    return (strcasecmp(ext_name, clean_ext) == 0);
}
