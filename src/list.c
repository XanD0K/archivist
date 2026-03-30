#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Headers
#include "commands.h"
#include "list.h"
#include "utils.h"

// Globals
static char *base_dir = NULL;
static int reverse;
static bool dir_first;
static bool ignore_case;

// Prototypes
static void print_list_help(void);
static ListOptions parse_list_opts(int argc, char **argv, int opt_start);
static SortList get_sort_func(char *sort);
static int cmp_name(const struct dirent **a, const struct dirent **b);
static int cmp_date(const struct dirent **a, const struct dirent **b);
static int cmp_size(const struct dirent **a, const struct dirent **b);
static int cmp_ext(const struct dirent **a, const struct dirent **b);
static int check_is_dir (const struct dirent *a, const struct dirent *b);
static void check_element(struct dirent *namelist, const char *current_path, size_t *f_counter,
                          size_t *dir_counter, size_t *slink_counter, size_t *err_counter,
                          off_t *total_size, SortList sorter, ListOptions opts);

// Lists information from a given directory
int handle_list(int argc, char **argv)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[2]))
    {
        // Prints help message for 'list' functionality
        print_list_help();
        return 0;
    }

    // Defines starting values
    const char *dir_path = NULL;
    int opt_start = 2;    
    if (argc >= 3 && argv[2][0] != '-')
    {
        dir_path = argv[2];
        opt_start = 3;
    }

    // Gets valid directory (default: .)
    base_dir = get_valid_directory(dir_path);
    if (!base_dir)
    {
        return 4;
    }

    // Parses CLI arguments
    ListOptions opts = parse_list_opts(argc, argv, opt_start);

    reverse = (opts.reverse) ? -1 : 1;
    dir_first = opts.dir_first;
    ignore_case = opts.base.ignore_case;

    const char *sorts[] = {"date", "extension", "name", "size", "version"};

    // Gets sorter function
    SortList sorter = cmp_name;
    if (strcasecmp(opts.base.sort, "name") != 0)
    {
        size_t len = sizeof(sorts)/sizeof(sorts[0]);
        // Checks for valid sort method
        if (!check_sort(opts.base.sort, sorts, len))
        {
            errno = EINVAL;
            fprintf(stderr, "Invalid sort argument: %s\n"
                            "Help for list command: ./archisvist list help\n"
                            "Usage: ./archivist list [DIRECTORY] [FLAGS]\n",
                            strerror(errno));
            return 5;
        }

        // Gets sort function for given sort method
        sorter = get_sort_func(opts.base.sort);
    }

    // Gets all elements in directory
    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, sorter);

    if (n == -1)
    {
        free(base_dir);
        perror("scandir");
        return 6;
    }

    size_t f_counter = 0, dir_counter = 0, slink_counter = 0, err_counter = 0;    
    off_t total_size = 0;

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            if (!opts.base.recursive)
            {
                // Prints file's name
                printf("%s\n", namelist[i]->d_name);
            }            
            // Recursively checks directory elements and updates counters
            check_element(namelist[i], base_dir, &f_counter, &dir_counter, &slink_counter,
                          &err_counter, &total_size, sorter, opts);
            free(namelist[i]);
        }
    }

    free(namelist);
    free(base_dir);

    // Prints f_counter, dir_counter and total_size variables
    printf("Directories: %zu\n"
           "Files: %zu\n"
           "Simbolic Links: %zu\n", dir_counter, f_counter, slink_counter);
    
    if (opts.base.human_readable)
    {
        const char *f_out = formatted_output(total_size);
        printf("Total size: %s\n", f_out);
    }
    else
    {
        printf("Total size: %jd bytes\n", (intmax_t)(total_size));
    }
    if (err_counter != 0)
    {
        printf("(Finished listing with %zu erros)\n", err_counter);
        return 7;
    }
    
    return 0;
}

// Prints explanation of 'list' functionality
static void print_list_help(void)
{
    puts(
        "Usage: ./archivist list [DIRECTORY] [FLAGS]\n"
        "\n"
        "DIRECTORY defaults to current directory (.)\n"
        "\n"
        "Flags:\n"
        "   -h | --human-readable\n"
        "       outputs size of files/dir in a more readable format\n"
        "       default: off\n"
        "   -i | --ignore-case\n"
        "       distinguish case\n"
        "       default: on\n"
        "   -r | --reverse\n"
        "       changes order (ascending | descending)\n"
        "       default: off (ascending)\n"
        "   -s | --sort <date|name|size|extension|version>\n"
        "       sorts output by:\n"
        "           date → compares last modification date\n"
        "           name → compares the ASCII value of each character\n"
        "           size → compares the size\n"
        "           extension → compares extension of each file\n"
        "           version → compares letters and numbers separately\n"
        "       default: name\n"
        "   -R | --recursive\n"
        "       also lists subdirectories\n"
        "       default: off\n"
        "   --dir-first\n"
        "       directories before files\n"
        "       default: off\n"
        "\n"
        "Examples:\n"
        "./archivist list\n"
        "./archivist list /folder\n"
        "./archivist list /folder -o size -R\n"
        "./archivist list /folder --dir-first --recursive --reverse\n"
        "./archivist list /folder -i --sort version\n"
        "\n"
        "All commands: ./archivist help"
    );
}

// Parses through CLI arguments for 'list' functionality
static ListOptions parse_list_opts(int argc, char **argv, int opt_start)
{
    // Declares structure
    ListOptions opts = {0};

    // Sets default values
    opts.base.sort = "name";
    opts.base.ignore_case = true;
    
    // Sets array of flags
    static struct option long_opts[] =
    {
        {"human-readable", no_argument, 0, 'h'},
        {"ignore-case", no_argument, 0, 'i'},
        {"reverse", no_argument, 0, 'r'},
        {"sort", required_argument, 0, 's'},
        {"recursive", no_argument, 0, 'R'},
        {"dir-first", no_argument, 0, 0},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "hirs:R";

    // Skips command and directory in CLI arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // Human readable
            case 'h':
            {
                opts.base.human_readable = true;
            }
            // Ignore case
            case 'i':
            {
                opts.base.ignore_case = false;
            }
            // Oder
            case 'o':
            {
                opts.base.sort = optarg;
                break;
            }
            // Reverse
            case 'r':
            {
                opts.reverse = true;
                break;
            }
            // Recursive
            case 'R':
            {
                opts.base.recursive = true;
                break;
            }
            // Long arguments
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "dir-first") == 0)
                {
                    opts.dir_first = true;
                }

                break;
            }
            // Error
            case '?':
            {
                break;
            }
        }
    }

    return opts;
}

// Returns specific sort function based on chosen sort method
static SortList get_sort_func(char *sort)
{
    if (strcasecmp(sort, "version") == 0)
    {
        return versionsort;
    }
    else if (strcasecmp(sort, "date") == 0)
    {
        return cmp_date;
    }
    else if (strcasecmp(sort, "size") == 0)
    {
        return cmp_size;
    }
    else if (strcasecmp(sort, "extension") == 0)
    {
        return cmp_ext;
    }

    // Fallback
    return cmp_name;
}

// Organizes by name
static int cmp_name(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir;
        }
    }

    int result = (!ignore_case) ? (strcmp((*a)->d_name, (*b)->d_name)) : (strcasecmp((*a)->d_name, (*b)->d_name)); 

    return result * reverse;
}

// Organizes by date
static int cmp_date(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir;
        }
    }

    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    // Constructs full path for current elements
    snprintf(pathA, sizeof(pathA), "%s/%s", base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", base_dir, (*b)->d_name); 

    // Gets element's data
    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (!ignore_case) ? (strcmp((*a)->d_name, (*b)->d_name)) :(strcasecmp((*a)->d_name, (*b)->d_name));
    }
    
    int result = (sa.st_mtime > sb.st_mtime) - (sa.st_mtime < sb.st_mtime);
    
    return result * reverse;
}

// Organizes by size
static int cmp_size(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir;
        }
    }

    struct stat sa, sb;
    char pathA[PATH_MAX], pathB[PATH_MAX];

    snprintf(pathA, sizeof(pathA), "%s/%s", base_dir, (*a)->d_name);
    snprintf(pathB, sizeof(pathB), "%s/%s", base_dir, (*b)->d_name);

    if (stat(pathA, &sa) != 0 || stat(pathB, &sb) != 0)
    {
        // Fallback to name if fails
        return (strcasecmp((*a)->d_name, (*b)->d_name));
    }

    int result = (sa.st_size > sb.st_size) - (sa.st_size < sb.st_size);
    
    return result * reverse;

}

// Organizes by extension
static int cmp_ext(const struct dirent **a, const struct dirent **b)
{
    // Directories before files
    if (dir_first)
    {
        int is_dir = check_is_dir(*a, *b);
        if (is_dir != 0)
        {
            return is_dir;
        }
    }

    const char *extA = get_extension((*a)->d_name);
    const char *extB = get_extension((*b)->d_name);

    int result = strcasecmp(extA, extB);

    return result * reverse;
}

// Helper function that puts directories before files
static int check_is_dir (const struct dirent *a, const struct dirent *b)
{
    int is_dir_a = (a->d_type == DT_DIR);
    int is_dir_b = (b->d_type == DT_DIR);

    // Folders before files
    if (is_dir_a != is_dir_b)
    {
        int result = is_dir_b - is_dir_a;
        return result * reverse;
    }

    return 0;
}

// Traverses through every element and updates counters
static void check_element(struct dirent *namelist, const char *current_path, size_t *f_counter,
                          size_t *dir_counter, size_t *slink_counter, size_t *err_counter,
                          off_t *total_size, SortList sorter, ListOptions opts)
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
        fprintf(stderr, "Couldn't access %s: %s\n", new_path, strerror(errno));
        (*err_counter)++;
        return;
    }

    // File
    if (S_ISREG(st.st_mode))
    {
        (*f_counter)++;
        *total_size += st.st_size;
    }
    // Simbolic Link
    else if (S_ISLNK(st.st_mode))
    {
        (*slink_counter)++;
    }
    // Directory
    else if (S_ISDIR(st.st_mode))
    {
        (*dir_counter)++;

        struct dirent **entry;
        int n = scandir(new_path, &entry, NULL, sorter);

        if (n == -1)
        {
            perror("scandir");
            (*err_counter)++;
        }

        for (int i = 0; i < n; i++)
        {
            // Recursively checks directory elements and updates counters
            check_element(entry[i], new_path, f_counter, dir_counter, slink_counter,
                          err_counter, total_size, sorter, opts);
            free(entry[i]);
        }

        free(entry);
    }

    if (opts.base.recursive)
    {
        // Prints file's name
        const char *sufix = new_path + strlen(base_dir);
        if (*sufix == '/')
        {
            sufix++;
        }
        printf("%s\n", sufix);
    }

    return;
}
