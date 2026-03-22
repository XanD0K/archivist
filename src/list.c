#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "commands.h"
#include "list.h"
#include "utils.h"

// Prototypes
static void print_list_help(void);
static ListOptions parse_list_opts(int argc, char **argv, int opt_start);
static bool check_sort(char *sort);
static SortFunc get_sort_func(char *order);
static int cmp_name(const struct dirent **a, const struct dirent **b);
static int cmp_date(const struct dirent **a, const struct dirent **b);
static int cmp_size(const struct dirent **a, const struct dirent **b);
static int cmp_type(const struct dirent **a, const struct dirent **b);
static int check_is_dir (const struct dirent *a, const struct dirent *b);
static void check_element(struct dirent *namelist, const char *current_path, size_t *f_counter,
                          size_t *dir_counter, size_t *slink_counter, size_t *err_counter,
                          size_t *total_size, SortFunc sorter, ListOptions opts);

// Globals
static const char *sorts[] = {"date", "name", "size", "type", "version"};
char *base_dir = NULL;
static int reverse;
static bool dir_first;
static bool case_sensitive;

// Lists information from a given directory
int handle_list(int argc, char **argv)
{
    // Checks for 'help' flag
    if (argc == 3 && 
        (strcasecmp(argv[2], "help") == 0 ||
         strcasecmp(argv[2], "--help") == 0 ||
         strcasecmp(argv[2], "-h") == 0))
    {
        print_list_help();
        return 0;
    }
    
    const char *dir_path = NULL;
    int opt_start = 0;    

    if (argc >= 3 && argv[2][0] != '-')
    {
        dir_path = argv[2];
        opt_start = 3;
    }
    else
    {
        opt_start = 2;
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
    case_sensitive = opts.case_sensitive;

    size_t f_counter = 0, dir_counter = 0, slink_counter = 0, err_counter = 0, total_size = 0;

    // Gets sorter function
    SortFunc sorter = cmp_name;
    if (strcasecmp(opts.order, "name") != 0)
    {
        // Checks for valid sort method
        if (!check_sort(opts.order))
        {
            errno = EINVAL;
            fprintf(stderr, "Invalid sort argument: %s\n"
                            "Usage: ./archivist list DIRECTORY [name|version|type|size|date] [asc|desc] [-r|--recursive] [-df|--dirfirst] [-cs|-ci]", strerror(errno));
            return 5;
        }

        // Gets sort function for given sort method
        sorter = get_sort_func(opts.order);
    }

    // Gets all elements in directory
    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, sorter);

    if (n == -1)
    {
        perror("scandir");
        return 6;
    }

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            if (!opts.recursive)
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
           "Simbolic Links: %zu\n"
           "Total size: %zu\n", dir_counter, f_counter, slink_counter, total_size);
    if (err_counter != 0)
    {
        printf("(Finish listing with %zu erros)\n", err_counter);
        return 7;
    }
    
    return 0;
}

// Prints explanations to 'list' functionality
static void print_list_help(void)
{
    puts(
        "Usage: ./archivist list DIRECTORY [FLAGS]\n"
        "\n"
        "Flags:\n"
        "   -o, --order <date|name|size|type|version>\n"
        "       Sorts output\n"
        "       Default: name\n"
        "   -r, --reverse\n"
        "       Changes order (ascending | descending)\n"
        "       Default: asc\n"
        "   -R, --recursive\n"
        "       Also lists subdirectories\n"
        "       Default: off\n"
        "   --dir-first\n"
        "       Directories before files\n"
        "       Default: off\n"
        "   --case-sensitive\n"
        "       Distinguish case\n"
        "       Default: off\n"
        "\n"
        "Examples:\n"
        "./archivist list /folder\n"
        "./archivist list /folder -o size -R\n"
        "./archivist list /folder --dir-first --recursive --reverse\n"
        "./archivist list /folder --case-sensitive --order version\n"
        "\n"
        "All commands: ./archivist help"
    );
}

// Parses through CLI arguments
static ListOptions parse_list_opts(int argc, char **argv, int opt_start)
{
    // Declares structure
    ListOptions opts = {0};

    // Sets default values
    opts.order = "name";
    
    // Sets array of flags
    static struct option long_opts[] =
    {
        {"order", required_argument, 0, 'o'},
        {"reverse", no_argument, 0, 'r'},
        {"recursive", no_argument, 0, 'R'},
        {"dir-first", no_argument, 0, 0},
        {"case-sensitive", no_argument, 0, 0},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *shor_opts = "o:rR";

    // Skips command and directory in CLI arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, shor_opts, long_opts, &long_index)) != -1)
    {
        switch(opt)
        {
            // Oder
            case 'o':
            {
                opts.order = optarg;
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
                opts.recursive = true;
                break;
            }

            // Long arguments
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "dir-first") == 0)
                {
                    opts.dir_first = true;
                }
                else if (strcmp(long_opts[long_index].name, "case-sensitive") == 0)
                {
                    opts.case_sensitive = true;
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

// Checks for valid sort method
static bool check_sort(char *sort)
{
    size_t sorts_len = sizeof(sorts) / sizeof(sorts[0]);

    for (size_t i = 0; i < sorts_len; i++)
    {
        if (strcasecmp(sort, sorts[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

// Returns specific sort function based on chosen sort method
static SortFunc get_sort_func(char *order)
{
    if (strcasecmp(order, "version") == 0)
    {
        return versionsort;
    }
    else if (strcasecmp(order, "date") == 0)
    {
        return cmp_date;
    }
    else if (strcasecmp(order, "size") == 0)
    {
        return cmp_size;
    }
    else if (strcasecmp(order, "type") == 0)
    {
        return cmp_type;
    }

    // Fallback
    return cmp_name;
}

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

    int result = (case_sensitive) ? (strcmp((*a)->d_name, (*b)->d_name)) : (strcasecmp((*a)->d_name, (*b)->d_name)); 

    return result * reverse;
}

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
        return (case_sensitive) ? (strcmp((*a)->d_name, (*b)->d_name)) :(strcasecmp((*a)->d_name, (*b)->d_name));
    }
    
    int result = (sa.st_mtime > sb.st_mtime) - (sa.st_mtime < sb.st_mtime);
    
    return result * reverse;
}

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

static int cmp_type(const struct dirent **a, const struct dirent **b)
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

    const char *extA = strrchr((*a)->d_name, '.');
    const char *extB = strrchr((*b)->d_name, '.');

    if (!extA)
    {
        extA = "";
    }

    if (!extB)
    {
        extB = "";
    }

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
                          size_t *total_size, SortFunc sorter, ListOptions opts)
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
        *total_size += (size_t)st.st_size;
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

    if (opts.recursive)
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
