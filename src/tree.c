#define _GNU_SOURCE

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
#include "help.h"
#include "utils.h"

// Prototypes
static void print_tree(struct dirent **namelist, char *base_dir, int n);
static void print_branch(struct dirent *namelist, char *current_path, const char *base_dir, char *prefix, bool is_last);
static char *concatenates_prefix(char *prefix, char *sufix);

// Displays directory's structure 
int handle_tree(int argc, char **argv, int min_args)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[min_args]))
    {
        print_tree_help();
        return 0;
    }

    // Gets valid base directory (default: .)
    char *base_dir = get_valid_directory(argv[min_args]);

    // Gets all content from directory
    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, versionsort);
    if (n == -1)
    {
        free(base_dir);
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        return 6;
    }
    
    print_tree(namelist, base_dir, n);

    free(base_dir);    
    return 0;
}

// Prints root and defines starting point for printing branches
static void print_tree(struct dirent **namelist, char *base_dir, int n)
{
    // Prints root
    printf("%s\n", base_dir);

    char *prefix = "";

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            bool is_last = (i == (n - 1));
            
            print_branch(namelist[i], base_dir, base_dir, prefix, is_last);
        }

        free(namelist[i]);
    }

    free(namelist);
}

// Prints all content from a diretory
static void print_branch(struct dirent *namelist, char *current_path, const char *base_dir, char *prefix, bool is_last)
{
    char *symbol = (is_last) ? "└── " : "├── ";

    char new_path[PATH_MAX];
    if (check_path_name_size(new_path, sizeof(new_path), current_path, namelist->d_name) == -1)
    {
        return;
    }

    struct stat st;
    if (stat(new_path, &st) != 0)
    {
        // Fallback to just printing the element
        printf("%s%s%s\n", prefix, symbol, namelist->d_name);
        return;
    }

    printf("%s%s%s\n", prefix, symbol, namelist->d_name);

    if (S_ISDIR(st.st_mode))
    {
        struct dirent **entry;
        int n = scandir(new_path, &entry, NULL, versionsort);
        if (n == -1)
        {
            return;
        }
        for (int i = 0; i < n; i++)
        {
            bool child_is_last = (i == n - 1);
            if (strcmp(entry[i]->d_name, ".") != 0 && strcmp(entry[i]->d_name, "..") != 0)
            {
                char *continuation = (is_last) ? "    " : "│   ";
                char *new_prefix = concatenates_prefix(prefix, continuation);
                if (!new_prefix)
                {
                    // Fallbacks to previous prefix
                    print_branch(entry[i], new_path, base_dir, prefix, child_is_last);
                }
                else
                {
                    print_branch(entry[i], new_path, base_dir, new_prefix, child_is_last);
                }

                free(new_prefix);
            }

            free(entry[i]);
        }

        free(entry);
    }
}

// Concatenates prefix
static char *concatenates_prefix(char *prefix, char *sufix)
{
    size_t prefix_len = strlen(prefix);
    size_t sufix_len = strlen(sufix);
    size_t total_size = prefix_len + sufix_len;

    char *new_prefix = calloc(total_size + 1, sizeof(char));
    if (!new_prefix)
    {
        return NULL;
    }

    strcpy(new_prefix, prefix);
    strcat(new_prefix, sufix);

    return new_prefix;
}
