#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "cli_parse_common.h"
#include "commands.h"
#include "help.h"
#include "rename.h"
#include "utils.h"
#include "utils_filter.h"
#include "utils_sort.h"

// Prototypes
static void rename_element(struct dirent *namelist, Extension *ext, size_t ext_counter,
                           const char *base_dir, const char *current_path, SortFlag sorter,
                           RenameOptions *opts, size_t *current_counter);
static char *generate_unique_name(char *current_path, char *old_name, char *input_name, size_t *current_counter);

int handle_rename(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_rename_help, parse_rename_options, sizeof(RenameOptions));
    if (!context)
    {
        return 10;
    }
    if (context->error_code != 0)
    {
        free_command_context(context);
        return (context->error_code == -1) ? 0 : context->error_code;
    }

    RenameOptions *opts = (RenameOptions*)context->opts;
    cmp_opts.base_dir = context->base_dir;

    SortFlag sorter = cmp_name_scandir;
    if (opts->base.sort && strcasecmp(opts->base.sort, "name") != 0)
    {
        const char *sorts[] = {"date", "name", "size", "version"};
        size_t len = sizeof(sorts) / sizeof(sorts[0]);

        // Updates sorter function
        sorter = get_sort_function(opts->base.sort, sorts, len);
        if (!sorter)
        {
            free_command_context(context);
            return 5;
        }
    }

    Extension *ext = NULL;
    size_t ext_counter = 0;
    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        ext = get_all_extensions(opts->filter.extension, &ext_counter);
        if (!ext)
        {
            free_command_context(context);
            errno = ENOMEM;
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            return 10;
        }
    }

    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, sorter);
    if (n == -1)
    {
        free_extensions(ext, ext_counter);
        free_command_context(context);
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        return 6;
    }

    size_t name_counter = 0;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            rename_element(namelist[i], ext, ext_counter, context->base_dir, context->base_dir, sorter, opts, &name_counter);
        }

        free(namelist[i]);
    }

    free(namelist);
}

// Parses through CLI arguments for 'rename' functionality
int parse_rename_options(int argc, char **argv, int opt_start, void *opts_out)
{
    RenameOptions *opts = (RenameOptions*)opts_out;
    opts->base.sort = "name";

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

    ret = parse_action_options(argc, argv, opt_start, &opts->action);
    if (ret != 0)
    {
        return ret;
    }   

    static struct option long_opts[] =
    {
        {"name", required_argument, 0, 'n'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0,  long_index = 0;
    char *short_opts = "n:";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'n':
            {
                opts->name = optarg;
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

// Renames files from given directory
static void rename_element(struct dirent *namelist, Extension *ext, size_t ext_counter,
                           const char *base_dir, const char *current_path, SortFlag sorter,
                           RenameOptions *opts, size_t *current_counter)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    char old_path[PATH_MAX];
    if (check_path_name_size(old_path, sizeof(old_path), current_path, namelist->d_name) == -1)
    {
        return;
    }

    struct stat st;
    if (stat(old_path, &st) != 0)
    {
        return;
    }

    if (S_ISDIR(st.st_mode) && opts->base.recursive)
    {
        struct dirent **entry;
        int n = scandir(old_path, &entry, NULL, sorter);
        if (n == -1)
        {
            return;
        }
        size_t name_counter = 0;
        for (int i = 0; i < n; i++)
        {
            rename_element(namelist, ext, ext_counter, base_dir, old_path, sorter, opts, name_counter);
            free(entry[i]);
        }

        free(entry);
    }

    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
    {
        return;
    }

    if (opts->filter.type && !match_type(opts->filter.type, st.st_mode))
    {
        return;
    }

    if (opts->filter.contains && opts->filter.contains[0] != '\0' &&
        !match_name(opts->filter.contains, namelist->d_name))
    {
        return;
    }

    // Checks for matching extension
    if (ext && !match_extension(ext, ext_counter, namelist->d_name))
    {
        return;
    }

    // Checks for size
    if ((opts->filter.max_size || opts->filter.min_size) &&
        !match_size(opts->filter.max_size, opts->filter.min_size, st.st_size))
    {
        return;
    }

    // Gets new name
    const char *new_path = match_existed_file(current_path, namelist->d_name, opts->name, &current_counter);
    if (!new_path)
    {
        return;
    }

    // Renames files
    rename(old_path, new_path);
    
    // Outputs

}

// Defines behaviour when file already exists on destination
static char *generate_unique_name(char *current_path, char *old_name, char *input_name, size_t *counter)
{
    // Default behavior: incremental rename
    const char *dot = strrchr(old_name, '.');
    const char *valid_dot = (dot != NULL) ? dot : "";

    char new_name[PATH_MAX];
    char full_path[PATH_MAX];

    while (1)
    {
        snprintf(new_name, sizeof(new_name), "%s_%zu%s", input_name, (*counter), valid_dot);

        if (check_path_name_size(full_path, sizeof(full_path), current_path, new_name) == -1)
        {
            return NULL;
        }

        if (access(full_path, F_OK) != 0)
        {
            return strdup(full_path);
        }

        (*counter)++;
    }
}
