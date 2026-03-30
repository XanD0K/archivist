#ifndef COMMANDS_H
#define COMMANDS_H

// Type/Alias
typedef int (*CommandHandler)(int argc, char **argv);

// Structure for each command in the table
typedef struct
{
    const char *name;           // Command's name
    CommandHandler handler;     // Command's function
    int min_args;               // Minimum number or arguments
    int max_args;               // Maximum number of arguments
} CommandEntry;

// Prototypes
int execute_command(int argc, char **argv);
void handle_help(void);  // help.c
int handle_tree(int argc, char **argv);  // tree.c

#endif
