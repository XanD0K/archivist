#include "archivist.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {        
        fprintf(stderr, "Usage: ./archivist COMMAND [DIRECTORY]");
        return 1;
    }

    // Checks for valid command
    char **command = validade_command(argv[1]);
    if (command == NULL)
    {
        fprintf(stderr, "Invalid command: %s!"
                        "Check all commands available with: ./archivist help\n", argv[1]);
        return 2;
    }

    // Gets command's index
    ptrdiff_t cmd_index = command - commands;

    if (!validate_args(argc, argv[1], cmd_index))
    {
        return 3;
    }

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
        DIR *dir_src = open_directory(argv[1]);
        if (dir_src == NULL)
        {
            return 6;
        }

        switch(cmd_index)
        {
            // Backup
            case 0:
            {
                break;
            }
            // Delete
            case 1:
            {
                break;
            }
            // Duplicate
            case 2:
            {
                break;
            }
            // Help
            case 3:
            {
                break;
            }
            // List
            case 4:
            {
                break;
            }
            // Log
            case 5:
            {
                break;
            }
            // Move
            case 6:
            {
                break;
            }
            // Rename
            case 7:
            {
                break;
            }
            // Report
            case 8:
            {
                break;
            }
            // Restore
            case 9:
            {
                break;
            }
            // Search
            case 10:
            {
                break;
            }
            // Tree
            case 11:
            {
                break;
            }
            // Fallback
            default:
            {
                fprintf(stderr, "Invalid command: %s!"
                                "Check all commands available with: ./archivist help\n", argv[1]);
                return 2;
            }
        }

        // Lists all files in directory
        if (strcasecmp(argv[1], "list") == 0)
        {
            
            if (!check_arguments(argc, argv[1]))
            {
                return 7;
            }
            const char *order = (argc = 4) ? argv[3] : "default";
            list_files(&st, dir_src, order);
        }
    }
}