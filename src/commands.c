#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

# include "commands.h"
# include "utils.h"

// Prototypes
static ptrdiff_t validate_command(const char *cmd);
static int str_cmp(const void *a, const void *b);
static bool validate_args(int argc, ptrdiff_t index);
static bool check_args_count(int argc, ptrdiff_t index);

// Table with available commands
const CommandEntry cmd_table[] = 
{
    {"backup", handle_backup, 4, 4},
    {"delete", handle_delete, 3, 4},
    {"duplicate", handle_duplicate, 3, 3},
    {"help", handle_help, 2, 2},
    {"list", handle_list, 3, 4},
    {"log", handle_log, 2, 2},
    {"move", handle_move, 4, 5},
    {"rename", handle_rename, 3, 5},
    {"report", handle_report, 3, 3},
    {"restore", handle_restore, 4, 5},
    {"search", handle_search, 3, 4},
    {"tree", handle_tree, 3, 3},
    {NULL, NULL, 0, 0}
};

int execute_command(int argc, char **argv)
{
    // Validates command by getting its index in the array
    ptrdiff_t index = validate_command(argv[1]);
    if (index == -1)
    {
        fprintf(stderr, "Invalid command: %s!"
                        "Check all commands available with: ./archivist help\n", argv[1]);
        return 2;
    }

    // Checks if correct number of arguments
    if (!validate_args(argc, index))
    {
        return 3;
    }

    // Initializes st and DIR to access 
    if (strcasecmp(argv[1], "log") != 0 && strcasecmp(argv[1], "help") != 0)
    {
        // Tries to fill st with directory's data
        struct stat st;
        if (stat(argv[2], &st) != 0)
        {
            fprintf(stderr, "Error in stat() for %s: %s\n", argv[2], strerror(errno));
            return 4;
        }

        // Checks if path is a directory
        if (!S_ISDIR(st.st_mode))
        {
            errno = ENOTDIR;
            fprintf(stderr, "Error accessing diretory %s: %s", argv[2], strerror(errno));
            return 5;
        }

        // Opens directory
        DIR *dir_src = open_directory(argv[2]);
        if (dir_src == NULL)
        {
            return 6;
        }
    }
}

// Tries to get command's index w/ binary search
static ptrdiff_t validate_command(const char *cmd)
{
    // Gets size of array
    size_t len = sizeof(cmd_table) / sizeof(cmd_table[0]) - 1;

    // Binary Search in the array
    const CommandEntry *result = bsearch(cmd, cmd_table, len, sizeof(CommandEntry), str_cmp);
    
    // Returns -1 if invalid command, otherwise command's index
    return (result == NULL) ? -1 : result - cmd_table;
}

// Helper function used by bsearch()
static int str_cmp(const void *a, const void *b)
{
    const char *cmd_name = (const char *)a;
    const CommandEntry *e = (const CommandEntry *)b;
    return strcasecmp(cmd_name, e->name);
}

// Checks if correct number of arguments for each command
static bool validate_args(int argc, ptrdiff_t index)
{
    const char *cmd = cmd_table[index].name;

    if (!check_args_count(argc, index))
    {
        switch (index)
        {
            // Backup
            case 0:
            {                
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY [order]\n", cmd);
                break;
            }
            // Delete / List / Search
            case 1:
            case 4:
            case 10:
            {
                if (strcasecmp(cmd, "list") == 0)
                {
                    fprintf(stderr, "Usage: ./archivist %s DIRECTORY [order]\n", cmd);
                }
                else
                {
                    fprintf(stderr, "Usage: ./archivist %s DIRECTORY [name/extension/type]\n", cmd);
                }
                break;
            }
            // Duplicate / Report / Tree
            case 2:
            case 8:
            case 11:
            {
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY\n", cmd);
                break;
            }
            // Help / Log
            case 3:
            case 5:
            {
                fprintf(stderr, "Usage: ./archivist %s\n", cmd);
                break;
            }
            // Move / Restore
            case 6:
            case 9:
            {
                if (strcasecmp(cmd, "move") == 0)
                {
                    fprintf(stderr, "Usage: ./archivist %s SRC_DIRECTORY DEST_DIRECTORY [name/extension/type/size]\n", cmd);
                }
                else
                {
                    fprintf(stderr, "Usage: ./archivist %s SRC_DIRECTORY DEST_DIRECTORY [version]\n", cmd);
                }                
                break;
            }
            // Rename
            case 7:
            {
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY [filename] [place]\n", cmd);
                break;
            }
            // Fallback
            default:
            {
                fprintf(stderr, "Invalid command: %s!"
                                "Check all commands available with: ./archivist help\n", cmd);
            }
        }

        return false;
    }

    return true;
}

// Checks if number of arguments is correct for given function
static bool check_args_count(int argc, ptrdiff_t index)
{
    int min = cmd_table[index].min_args;
    int max = cmd_table[index].max_args;

    if (argc < min || argc > max)
    {
        return false;
    }

    return true;
}