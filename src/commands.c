// Libraries
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Headers
#include "commands.h"
#include "help.h"
#include "list.h"
#include "move.h"
#include "report.h"
#include "search_cmd.h"
#include "utils.h"

// Prototypes
static ptrdiff_t validate_command(const char *cmd, const CommandEntry cmd_table[], size_t len);
static int str_cmp(const void *a, const void *b);
static bool validate_args(int argc, ptrdiff_t index, const CommandEntry cmd_table[]);
static bool check_args_count(int argc, ptrdiff_t index, const CommandEntry cmd_table[]);

// Validades and executes commands
int execute_command(int argc, char **argv)
{
    if (argc < 2)
    {        
        fprintf(stderr, "Usage: ./archivist COMMAND [arguments]\n"
                        "Check available commands with: ./archivist help\n");
        return 1;
    }

    // Calls 'help' functionality
    if (strcasecmp(argv[1], "help") == 0)
    {
        if (argc != 2)
        {
            fprintf(stderr, "Usage: ./archivist help\n");
            return 1;
        }
        handle_help();
        return 0;
    }

    // Table with available commands
    const CommandEntry cmd_table[] = 
    {
        {"list", handle_list, 2, 8},
        {"move", handle_move, 3, 20},
        {"report", handle_report, 2, 9},
        {"search", handle_search, 3, 11},
        {"tree", handle_tree, 2, 3},
        {NULL, NULL, 0, 0}

        /*
        {"backup", handle_backup, 4, 4},
        {"delete", handle_delete, 3, 4},
        {"duplicate", handle_duplicate, 3, 3},
        {"log", handle_log, 2, 2},
        {"rename", handle_rename, 3, 5},
        {"restore", handle_restore, 4, 5},
        */
    };

    // Gets size of array
    size_t len = sizeof(cmd_table) / sizeof(cmd_table[0]) - 1;

    // Checks if command if valid
    ptrdiff_t index = validate_command(argv[1], cmd_table, len);
    if (index == -1)
    {
        fprintf(stderr, "Invalid command: %s!"
                        "Check available commands with: ./archivist help\n", argv[1]);
        return 2;
    }

    // Checks number of arguments
    if (!validate_args(argc, index, cmd_table))
    {
        return 3;
    }
    
    // Calls handler function
    const int handler_result = cmd_table[index].handler(argc, argv);
    return handler_result;
}

// Tries to get command's index
static ptrdiff_t validate_command(const char *cmd, const CommandEntry cmd_table[], size_t len)
{
    // Binary Search in the array
    const CommandEntry *result = bsearch(cmd, cmd_table, len, sizeof(CommandEntry), str_cmp);
    
    // Returns -1 if invalid command, otherwise returns command's index
    return (result == NULL) ? -1 : result - cmd_table;
}

// Helper function used by bsearch()
static int str_cmp(const void *a, const void *b)
{
    const char *cmd_name = (const char *)a;
    const CommandEntry *e = (const CommandEntry *)b;
    return strcasecmp(cmd_name, e->name);
}

// Checks correct number of arguments for each command
static bool validate_args(int argc, ptrdiff_t index, const CommandEntry cmd_table[])
{
    const char *cmd = cmd_table[index].name;

    if (!check_args_count(argc, index, cmd_table))
    {
        switch (index)
        {
            // Backup
            case 0:
            {
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY [FLAGS]\n", cmd);
                break;
            }
            // Delete / Search
            case 1:
            case 10:
            {
                fprintf(stderr, "Usage: ./archivist %s [DIRECTORY] [FLAGS]\n", cmd);
                break;
            }
            // Duplicate / Report / Tree
            case 2:
            case 8:
            case 11:
            {
                fprintf(stderr, "Usage: ./archivist %s [DIRECTORY]\n", cmd);
                break;
            }
            // Help / Log
            case 3:
            case 5:
            {
                fprintf(stderr, "Usage: ./archivist %s\n", cmd);
                break;
            }
            // List
            case 4:
            {
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY [order] [revers] [recursive] [dir-first] [case-insensitive]\n"
                                "See all flags available with: ./archivist %s -h|--help|help\n", cmd, cmd);
                break;
            }
            // Move / Restore
            case 6:
            case 9:
            {
                if (strcasecmp(cmd, "move") == 0)
                {
                    fprintf(stderr, "Usage: ./archivist %s SRC_DIRECTORY DEST_DIRECTORY [name|extension|type|size]\n", cmd);
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
static bool check_args_count(int argc, ptrdiff_t index, const CommandEntry cmd_table[])
{
    int min = cmd_table[index].min_args;
    int max = cmd_table[index].max_args;

    return (argc >= min && argc <= max);
}
