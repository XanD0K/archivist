#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "utils.h"

// Prototypes
static int str_cmp(const void *a, const void *b);


// All available commands
char *commands[] = {"backup", "delete", "duplicate", "help", "list", "log", "move", "rename", "report", "restore", "search", "tree"};

// Checks if valid command and returns its pointer
char **validade_command(char *command)
{
    // Gets size of array
    size_t len = sizeof(commands) / sizeof(commands[0]);

    // Binary Search in the array
    char **result = bsearch(&command, commands, len, sizeof(commands[0]), str_cmp);

    return result;
}

// Checks if correct number of arguments for each command
bool validate_args(int argc, char *command, ptrdiff_t index)
{
    switch (index)
    {
        // Backup
        case 0:
        {
            if (argc != 4)
            {
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY [order]\n", command);
                return false;
            }
            break;
        }
        // Delete / List / Search
        case 1:
        case 4:
        case 10:
        {
            if (argc < 3  || argc > 4)
            {
                if (strcasecmp(command, "list") == 0)
                {
                    fprintf(stderr, "Usage: ./archivist %s DIRECTORY [order]\n", command);
                }
                else
                {
                    fprintf(stderr, "Usage: ./archivist %s DIRECTORY [name/extension/type]\n", command);
                }
                return false;
            }
            break;
        }
        // Duplicate / Report / Tree
        case 2:
        case 8:
        case 11:
        {
            if (argc != 3)
            {
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY\n", command);
                return false;
            }
            break;
        }
        // Help / Log
        case 3:
        case 5:
        {
            if (argc != 2)
            {
                fprintf(stderr, "Usage: ./archivist %s\n", command);
                return false;
            }
            break;
        }
        // Move / Restore
        case 6:
        case 9:
        {
            if (argc < 4 || argc > 5)
            {
                if (strcasecmp(command, "move") == 0)
                {
                    fprintf(stderr, "Usage: ./archivist %s SRC_DIRECTORY DEST_DIRECTORY [name/extension/type/size]\n", command);
                }
                else
                {
                    fprintf(stderr, "Usage: ./archivist %s SRC_DIRECTORY DEST_DIRECTORY [version]\n", command);
                }                
                return false;
            }
            break;
        }
        // Rename
        case 7:
        {
            if (argc < 3 || argc > 5)
            {
                fprintf(stderr, "Usage: ./archivist %s DIRECTORY [filename] [place]\n", command);
                return false;
            }
            break;
        }
        // Fallback
        default:
        {
            fprintf(stderr, "Invalid command: %s!"
                            "Check all commands available with: ./archivist help\n", command);
            return false;
        }
    }
    return true;
}

// Opens directory
DIR *open_directory(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr, "Couldn't open directory %s: %s\n", path, strerror(errno));
        return NULL;
    }
    return dir;
}

// Helper function used by bsearch()
static int str_cmp(const void *a, const void *b)
{
    return strcasecmp(*(const char **)a, *(const char **)b);
}
