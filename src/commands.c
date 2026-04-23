// Libraries
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>  // ssize_t

// Headers
#include "backup.h"
#include "commands.h"
#include "delete.h"
#include "help.h"
#include "list.h"
#include "log.h"
#include "move.h"
#include "recover.h"
#include "rename.h"
#include "report.h"
#include "search_cmd.h"
#include "utils.h"

// Prototypes
static ErrorCode validate_command(const char *cmd, const CommandEntry cmd_table[], size_t len, size_t *index);
static bool validate_args(int argc, size_t index, const CommandEntry cmd_table[]);
static bool check_args_count(int argc, size_t index, const CommandEntry cmd_table[]);

// Validades and executes commands
ErrorCode execute_command(int argc, char **argv)
{
    if (argc < 2)
    {        
        fprintf(stderr, "Usage: ./archivist COMMAND [arguments]\n"
                        "Check available commands with: ./archivist help\n");
        return EC_INVALID_ARGC;
    }

    // Calls 'help' functionality
    if (strcasecmp(argv[1], "help") == 0)
    {
        if (argc != 2)
        {
            fprintf(stderr, "Usage: ./archivist help\n");
            return EC_INVALID_ARGC;
        }
        handle_help();
        return EC_HELP_FLAG;
    }

    // Table with available commands
    const CommandEntry cmd_table[] = 
    {
        {"backup", handle_backup, 3, 17},
        {"delete", handle_delete, 2, 18},
        {"list", handle_list, 2, 8},
        {"log", handle_log, 2, 6},    
        {"move", handle_move, 3, 20},
        {"recover", handle_recover, 3, 8},
        {"rename", handle_rename, 2, 18},        
        {"report", handle_report, 2, 9},
        {"search", handle_search, 2, 11},
        {"tree", handle_tree, 2, 3},
        {NULL, NULL, 0, 0}
    };

    // Gets size of array
    size_t len = sizeof(cmd_table) / sizeof(cmd_table[0]) - 1;

    // Checks if command if valid
    size_t index = 0;
    ErrorCode ret = validate_command(argv[1], cmd_table, len, &index);
    if (ret == EC_INVALID_COMMAND)
    {
        fprintf(stderr, "Invalid command: %s!"
                        "Check available commands with: ./archivist help\n", argv[1]);
        return ret;
    }

    // Checks number of arguments for given command
    if (!validate_args(argc, index, cmd_table))
    {
        return EC_INVALID_COMMAND_ARGC;
    }
    
    // Calls handler function
    const ErrorCode handler_result = cmd_table[index].handler(argc, argv, cmd_table[index].min_args);
    return handler_result;
}

// Checks if command is valid
static ErrorCode validate_command(const char *cmd, const CommandEntry cmd_table[], size_t len, size_t *index)
{
    for (size_t i = 0; i < len; i++)
    {
        int cmp = strcasecmp(cmd, cmd_table[i].name);
        if (cmp == 0)
        {
            *index = i;
        }
        if (cmp < 0)
        {
            break;
        }
    }
    
    // Invalid command
    return EC_INVALID_COMMAND;
}

// Checks correct number of arguments for each command
static bool validate_args(int argc, size_t index, const CommandEntry cmd_table[])
{
    const char *cmd = cmd_table[index].name;

    if (!check_args_count(argc, index, cmd_table))
    {
        switch (index)
        {
            case 0:  // Backup
            case 5:  // Move
            case 6:  // Recover
            {
                fprintf(stderr, "Usage: ./archivist %s [DIRECTORY] DIRECTORY [FLAGS]\n", cmd);
                break;
            }
            case 1:  // Delete
            case 3:  // List
            case 7:  // Rename
            case 8:  // Report
            case 9:  // Search
            {
                fprintf(stderr, "Usage: ./archivist %s [DIRECTORY] [FLAGS]\n", cmd);
                break;
            }
            case 2:  // Help
            {
                fprintf(stderr, "Usage: ./archivist %s\n", cmd);
                break;
            }
            case 4:  // Log
            {
                fprintf(stderr, "Usage: ./archivist %s [FLAGS]\n", cmd);
                break;
            }
            case 10:  // Tree
            {
                fprintf(stderr, "Usage: ./archivist %s [DIRECTORY]\n", cmd);
                break;
            }
            // Fallback
            default:
            {
                fprintf(stderr, "Invalid command: %s!\n"
                        "Check all commands available with: ./archivist help\n", cmd);
            }
        }

        return false;
    }

    return true;
}

// Checks if number of arguments is correct for given function
static bool check_args_count(int argc, size_t index, const CommandEntry cmd_table[])
{
    int min = cmd_table[index].min_args;
    int max = cmd_table[index].max_args;

    return (argc >= min && argc <= max);
}

// Configures and Validates features' initial data
CommandContext *setup_command(int argc, char **argv, int min_args, PrintHelp print_help,
                              ParseOptions parser, size_t opts_size)
{
    CommandContext *context = calloc(1, sizeof(CommandContext));
    if (!context)
    {
        errno = ENOMEM;
        fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
        return NULL;
    }

    // Checks for 'help' flag
    if (check_help(argc, argv[2]))
    {
        print_help();
        context->error_code = EC_CMD_HELP_FLAG;
        return context;
    }

    // Defines starting values
    const char *src_path = NULL;
    const char *dst_path = NULL;
    context->opt_start = min_args;

    // Features that only use source directory
    if (min_args == 2)
    {
        if (argc > min_args && argv[min_args][0] != '-')
        {
            src_path = argv[min_args];
            context->opt_start = min_args + 1;
        }
    }

    // Features that use source directory and destination directory
    else if (min_args == 3)
    {
        if (argc == min_args && argv[min_args - 1][0] != '-')
        {
            dst_path = argv[min_args - 1];
        }
        else if (argc > min_args && argv[min_args][0] == '-')
        {
            dst_path = argv[min_args - 1];
        }
        if (argc > min_args && argv[min_args][0] != '-')
        {
            src_path = argv[min_args - 1];
            dst_path = argv[min_args];
            context->opt_start = min_args + 1;
        }
    }

    // Validates source directory
    context->base_dir = get_valid_directory(src_path);
    if (!context->base_dir)
    {
        context->error_code = EC_INVALID_DIRECTORY;
        return context;
    }

    // Validates destination directory
    if (min_args == 3)
    {
        context->dst_dir = get_valid_destination(dst_path);
        if (!context->dst_dir)
        {
            context->error_code = EC_INVALID_DIRECTORY;
            return context;
        }
    }

    context->opts = calloc(1, opts_size);
    if (!context->opts)
    {
        errno = ENOMEM;
        fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
        context->error_code = EC_MEMORY_ALLOCATION;
        return context;
    }
    ErrorCode parse_err = parser(argc, argv, context->opt_start, context->opts);
    if (parse_err != EC_SUCCESS)
    {
        context->error_code = parse_err;
        return context;
    }

    context->error_code = EC_SUCCESS;
    return context;
}

// Free command setup's structure
void free_command_context(CommandContext *context)
{
    if (!context)
    {
        return;
    }

    if (context->base_dir)
    {
        free(context->base_dir);
    }
    if (context->dst_dir)
    {
        free(context->dst_dir);
    }
    if (context->opts)
    {
        free(context->opts);
    }
    free(context);
}
