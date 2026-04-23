#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

// Libraries
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Header
#include "error_code.h"
#include "help.h"
#include "utils.h"

// Prototypes
static void print_tree(struct dirent **namelist, const char *base_dir, int n);
static void print_branch(const struct dirent *namelist, const char *current_path,
                         const char *base_dir, const char *prefix, bool is_last);
static char *concatenates_prefix(const char *prefix, const char *sufix);

// Setup logic for 'tree' feature
ErrorCode handle_tree(int argc, char **argv, int min_args)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[min_args]))
    {
        print_tree_help();
        return EC_SUCCESS;
    }

    // Gets valid base directory (default: .)
    char *base_dir = get_valid_directory(argv[min_args]);
    if (!base_dir)
    {
        return EC_INVALID_DIRECTORY;
    }

    // Gets all content from directory
    struct dirent **namelist = NULL;
    int n = scandir(base_dir, &namelist, scandir_no_dot_filter, versionsort);
    if (n == -1)
    {
        free(base_dir);
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        return EC_SCANDIR_ERROR;
    }
    
    print_tree(namelist, base_dir, n);

    free(base_dir);    
    return EC_SUCCESS;
}

// Prints root and defines starting point for branches
static void print_tree(struct dirent **namelist, const char *base_dir, int n)
{
    // Prints root
    printf("%s\n", base_dir);

    const char *prefix = "";

    for (int i = 0; i < n; i++)
    {
        bool is_last = (i == (n - 1));
        print_branch(namelist[i], base_dir, base_dir, prefix, is_last);

        free(namelist[i]);
    }

    free(namelist);
}

// Prints all content from a diretory
static void print_branch(const struct dirent *namelist, const char *current_path,
                         const char *base_dir, const char *prefix, bool is_last)
{
    char *symbol = (is_last) ? "└── " : "├── ";
    char *continuation = (is_last) ? "    " : "│   ";

    char new_path[PATH_MAX];
    if (check_path_name_size(new_path, sizeof(new_path), current_path, namelist->d_name) == -1)
    {
        fprintf(stderr, "Path too long: %s/%s\n", current_path, namelist->d_name);
        return;
    }

    printf("%s%s%s\n", prefix, symbol, namelist->d_name);

    bool is_dir = (namelist->d_type == DT_DIR);
    if (!is_dir && namelist->d_type == DT_UNKNOWN)
    {
        struct stat st;
        if (lstat(new_path, &st) == 0)
        {
            is_dir = S_ISDIR(st.st_mode);
        }
    }

    if (is_dir)
    {
        struct dirent **entry;
        int n = scandir(new_path, &entry, scandir_no_dot_filter, versionsort);
        if (n == -1)
        {
            return;
        }

        for (int i = 0; i < n; i++)
        {
            bool child_is_last = (i == n - 1);
            char *new_prefix = concatenates_prefix(prefix, continuation);
            if (new_prefix)
            {
                print_branch(entry[i], new_path, base_dir, new_prefix, child_is_last);
                free(new_prefix);

            }
            else
            {
                // Fallbacks to previous prefix
                print_branch(entry[i], new_path, base_dir, prefix, child_is_last);
            }

            free(entry[i]);
        }

        free(entry);
    }
}

// Concatenates prefix
static char *concatenates_prefix(const char *prefix, const char *sufix)
{
    size_t prefix_len = strlen(prefix);
    size_t sufix_len = strlen(sufix);
    size_t total_size = prefix_len + sufix_len;

    char *new_prefix = calloc(total_size + 1, sizeof(char));
    if (!new_prefix)
    {
        return NULL;
    }

    memcpy(new_prefix, prefix, prefix_len);
    memcpy(new_prefix + prefix_len, sufix, sufix_len + 1);

    return new_prefix;
}
