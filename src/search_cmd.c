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
#include "cli_parse_common.h"
#include "commands.h"
#include "help.h"
#include "search_cmd.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void search_element(char *current_path, const char *base_dir, SearchOptions *opts, const struct dirent *namelist, const char *searched, size_t *counter, bool *printed);
static bool match_searched_name(const char *current_name, const char *searched, bool contains, bool ignore_case);
static bool match_searched_extension(const char *current_name, const char *ext);

// Searches for a specific file/directory
int handle_search(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_search_help,
                                            parse_search_opts, sizeof(SearchOptions));
    if (!context)
    {
        return 10;
    }
    if (context->error_code != 0)
    {
        free_command_context(context);
        return (context->error_code == -1) ? 0 : context->error_code;
    }

    SearchOptions *opts = (SearchOptions*)context->opts;

    char *searched_name = strdup(argv[2]);
    if (!searched_name)
    {
        fprintf(stderr, "Error on strdup(): %s\n", strerror(errno));
        free_command_context(context);
        return 8;
    }

    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, alphasort);

    if (n == -1)
    {
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        free_command_context(context);
        return 6;
    }

    size_t counter = 0;
    bool printed = false;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            search_element(context->base_dir, context->base_dir, opts, namelist[i], searched_name, &counter, &printed);
        }

        free(namelist[i]);
    }

    if (!printed)
    {
        if (!opts->base.recursive)
        {
            printf("Couldn't find '%s' on '%s' base directory", searched_name, context->base_dir);
        }
        else
        {
            printf("Couldn't find '%s' on base directory '%s' and in any other of its subdirectory", searched_name, context->base_dir);
        }        
    }

    free(namelist);
    free(searched_name);
    free_command_context(context);
    return 0;
}

// Parses through CLI arguments for 'search' functionality
int parse_search_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    SearchOptions *opts = (SearchOptions*)opts_out;
    opts->base.ignore_case = true;

    int ret;
    ret = parse_common_opts(argc, argv, opt_start, &opts->base);
    if (ret != 0)
    {
        return ret;
    }
    ret = parse_filter_options(argc, argv, opt_start, &opts->filter);
    if (ret != 0)
    {
        if (ret == PARSE_ERROR_INVALID_SIZE)
        {
            errno = EIO;
            fprintf(stderr, "Invalid size: %s\n", strerror(errno));
            return 9;
        }
        return ret;
    }

    static struct option long_opts[] =
    {
        {"contains", no_argument, 0, 'c'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "c";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'c':
            {
                opts->contains = true;
                break;
            }
            case '?':
            {
                break;
            }
        }
    }
    
    return 0;

}

// Searches for an element
static void search_element(char *current_path, const char *base_dir, SearchOptions *opts, const struct dirent *namelist, const char *searched, size_t *counter, bool *printed)
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

    // Checks recursively for searched element
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
            search_element(new_path, base_dir, opts, entry[i], searched, counter, printed);
            free(entry[i]);
        }

        free(entry);
    }

    // Checks for name equality
    if (strcmp(namelist->d_name, "*") != 0 && !match_searched_name(namelist->d_name, searched, opts->contains, opts->base.ignore_case))
    {
        return;
    }

    // Checks for extension
    if (opts->filter.extension && !match_searched_extension(opts->filter.extension, namelist->d_name))
    {
        return;
    }

    // Checks for element's type
    if (opts->filter.type && !match_type(opts->filter.type, st.st_mode))
    {
        return;
    }

    // Checks for size
    if ((opts->filter.max_size || opts->filter.min_size) && !match_size(opts->filter.max_size, opts->filter.min_size, st.st_size))
    {
        return;
    }

    // Prints found element
    const char *suffix = get_suffix(new_path, base_dir);
    printf("%s\n", suffix);
    (*counter)++;
    *printed = true;
    
    return;
}

// Checks if strings match
static bool match_searched_name(const char *current_name, const char *searched, bool contains, bool ignore_case)
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

static bool match_searched_extension(const char *current_name, const char *ext)
{
    const char *ext_name = get_clean_extension(current_name);
    if (!ext_name || ext_name[0] == '\0')
    {
        return false;
    }

    const char *clean_ext = (strlen(ext) > 1 && ext[0] == '.') ? ext + 1 : ext;

    return (strcasecmp(ext_name, clean_ext) == 0);
}
