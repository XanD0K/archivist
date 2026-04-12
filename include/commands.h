#ifndef COMMANDS_H
#define COMMANDS_H

// Libraries
#include <stdbool.h>
#include <stddef.h>  // size_t

// Type/Alias
typedef int (*CommandHandler)(int argc, char **argv, int min_args);
typedef void (*PrintHelp)(void);
typedef int (*ParseOptions)(int argc, char **argv, int opt_start, void *opts_out);

// Structure for each command in the table
typedef struct
{
    const char *name;           // Command's name
    CommandHandler handler;     // Command's function
    int min_args;               // Minimum number or arguments
    int max_args;               // Maximum number of arguments
} CommandEntry;

// Structure to hande setup of each command
typedef struct {
    int error_code;
    char *base_dir;
    char *dst_dir;
    int opt_start;
    void *opts;
} CommandContext;

// Prototypes
int execute_command(int argc, char **argv);
int handle_tree(int argc, char **argv, int min_args);  // tree.c
CommandContext *setup_command(int argc, char **argv, int min_args, PrintHelp print_help,
                              ParseOptions parser, size_t opts_size);
void free_command_context(CommandContext *context);

#endif
