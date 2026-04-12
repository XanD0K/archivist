#define _GNU_SOURCE

// Libraries
#include <ctype.h>
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
#include "cli_parse_common.h"
#include "commands.h"
#include "help.h"
#include "list.h"
#include "utils.h"
#include "utils_sort.h"

// Prototypes
static void list_element(struct dirent *namelist, const char *base_dir, const char *current_path,
                         size_t *f_counter, size_t *dir_counter, size_t *slink_counter, size_t *err_counter,
                          off_t *total_size, SortFlag sorter, ListOptions *opts);

// Lists information from a given directory
int handle_list(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_list_help,
                                            parse_list_opts, sizeof(ListOptions));
    if (!context)
    {
        return 10;
    }
    if (context->error_code != 0)
    {
        free_command_context(context);
        return (context->error_code == -1) ? 0 : context->error_code;
    }

    ListOptions *opts = (ListOptions*)context->opts;

    // Redefines values for comparation variables
    cmp_opts.reverse = (opts->reverse) ? -1 : 1;
    cmp_opts.dir_first = opts->dir_first;
    cmp_opts.ignore_case = opts->base.ignore_case;
    cmp_opts.base_dir = context->base_dir;

    // Default sorter function
    SortFlag sorter = cmp_name_scandir;
    if (opts->base.sort && strcasecmp(opts->base.sort, "name") != 0)
    {
        const char *sorts[] = {"date", "extension", "name", "size", "version"};
        size_t len = sizeof(sorts)/sizeof(sorts[0]);
        
        // Updates sorter function
        sorter = get_sort_function(opts->base.sort, sorts, len);
        if (!sorter)
        {
            free_command_context(context);
            return 5;
        }
    }

    // Gets directory's content
    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, sorter);
    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        free_command_context(context);
        return 6;
    }

    // Initializes counters
    size_t f_counter = 0, dir_counter = 0, slink_counter = 0, err_counter = 0;    
    off_t total_size = 0;

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            if (!opts->base.recursive)
            {
                // Prints file's name
                printf("%s\n", namelist[i]->d_name);
            }            
            // Recursively checks directory elements and updates counters
            list_element(namelist[i], context->base_dir, context->base_dir, &f_counter, &dir_counter,
                         &slink_counter, &err_counter, &total_size, sorter, opts);
        }
        
        free(namelist[i]);
    }

    free(namelist);

    // Prints f_counter, dir_counter and total_size variables
    printf("Directories: %zu\n"
           "Files: %zu\n"
           "Simbolic Links: %zu\n", dir_counter, f_counter, slink_counter);
    
    if (opts->base.human_readable)
    {
        char *f_out = formatted_output(total_size);
        printf("Total size: %s\n", f_out);
        free(f_out);
    }
    else
    {
        printf("Total size: %jd bytes\n", (intmax_t)(total_size));
    }
    if (err_counter != 0)
    {
        printf("(Finished listing with %zu erros)\n", err_counter);
        free_command_context(context);
        return 7;
    }

    free_command_context(context);
    return 0;
}

// Parses through CLI arguments for 'list' functionality
int parse_list_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    // Declares structure
    ListOptions *opts = (ListOptions*)opts_out;

    // Sets default values
    opts->base.sort = "name";
    opts->base.ignore_case = true;

    int ret = parse_common_opts(argc, argv, opt_start, &opts->base);
    if (ret != 0)
    {
        return ret;
    }

    // Sets array of flags
    static struct option long_opts[] =
    {
        {"reverse", no_argument, 0, 'r'},
        {"dir-first", no_argument, 0, 0},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "r";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // Reverse
            case 'r':
            {
                opts->reverse = true;
                break;
            }
            // Long arguments
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "dir-first") == 0)
                {
                    opts->dir_first = true;
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

    return 0;
}

// Lists current element and updates counters
static void list_element(struct dirent *namelist, const char *base_dir, const char *current_path,
                         size_t *f_counter, size_t *dir_counter, size_t *slink_counter, size_t *err_counter,
                          off_t *total_size, SortFlag sorter, ListOptions *opts)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    char new_path[PATH_MAX];
    if (check_path_name_size(new_path, sizeof(new_path), current_path, namelist->d_name) == -1)
    {
        (*err_counter)++;
        return;
    }

    struct stat st;
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
            fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
            (*err_counter)++;
        }

        for (int i = 0; i < n; i++)
        {
            // Recursively checks directory elements and updates counters
            list_element(entry[i], base_dir, new_path, f_counter, dir_counter, slink_counter,
                          err_counter, total_size, sorter, opts);
            free(entry[i]);
        }

        free(entry);
    }

    if (opts->base.recursive)
    {
        // Prints file's name
        const char *suffix = get_suffix(new_path, base_dir);
        printf("%s\n", suffix);
    }

    return;
}
