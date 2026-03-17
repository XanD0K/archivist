#ifndef COMMANDS_H
#define COMMANDS_H

// Type for each command function
typedef int (*CommandHandler)(int argc, char **argv);

// Structure for each command in the table
typedef struct
{
    const char *name;
    CommandHandler handler;
    int min_args;
    int max_args;
} CommandEntry;

// Globals
extern const CommandEntry cmd_table[];

// Prototypes
int execute_command(int argc, char **argv);
int handle_backup(int argc, char **argv);
int handle_delete(int argc, char **argv);
int handle_duplicate(int argc, char **argv);
int handle_help(int argc, char **argv);
int handle_list(int argc, char **argv);
int handle_log(int argc, char **argv);
int handle_move(int argc, char **argv);
int handle_rename(int argc, char **argv);
int handle_report(int argc, char **argv);
int handle_restore(int argc, char **argv);
int handle_search(int argc, char **argv);
int handle_tree(int argc, char **argv);

#endif
