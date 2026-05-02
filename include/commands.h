#ifndef COMMANDS_H
#define COMMANDS_H

// Libraries
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>  // size_t

// Headers
#include "error_code.h"

// Type/Alias
typedef ErrorCode (*CommandHandler)(int argc, char **argv, int min_args);  // Feature's settup function
typedef void (*PrintHelp)(void);  // Festure's help function
typedef ErrorCode (*ParseOptions)(int argc, char **argv, int opt_start, void *opts_out);  // Feature's parser
typedef int (*ScandirFilter)(const struct dirent *);  // Filter used on scandir()

// Structure for each command in the table
typedef struct
{
    const char *name;           // Command's name
    CommandHandler handler;     // Command's function
    int min_args;               // Minimum number or arguments
    int max_args;               // Maximum number of arguments
} CommandEntry;

// Structure to handle setup of each command
typedef struct {
    char *base_dir;
    char *dst_dir;
    int opt_start;
    void *opts;
    ScandirFilter filter;
    ErrorCode error_code;
} CommandContext;

// Prototypes
ErrorCode execute_command(int argc, char **argv);
ErrorCode handle_tree(int argc, char **argv, int min_args);  // tree.c
CommandContext *setup_command(int argc, char **argv, int min_args, PrintHelp print_help,
                              ParseOptions parser, size_t opts_size);
void free_command_context(CommandContext *context);

#endif
