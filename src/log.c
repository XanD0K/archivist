#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Headers
#include "commands.h"
#include "help.h"
#include "log.h"
#include "utils.h"

// Prototypes
static const char *get_log_path(void);
static LogOptions parse_log_opts(int argc, char **argv, int opt_start);
static int parse_int(char *input);
static ErrorCode print_log(const char *log_dir, LogOptions opts);
static bool check_cmd_line(const char *cmd);


ErrorCode handle_log(int argc, char **argv, int min_args)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[min_args]))
    {
        print_tree_help();
        return EC_CMD_HELP_FLAG;
    }

    // Parses CLI arguments
    LogOptions opts = parse_log_opts(argc, argv, min_args);
    if (opts.limit == -1)
    {
        return EC_PARSE_ERROR;
    }

    // Gets directory where .log file is (default: ~/.local/share/archivist/)
    const char *log_dir = get_log_path();
    if (!log_dir)
    {
        return EC_INVALID_DIRECTORY;
    }

    return print_log(log_dir, opts);
}

static const char *get_log_path(void)
{
    const char *home = getenv("HOME");
    // Fallbacks to current directory (.)
    if (!home)
    {
        home = ".";
    }

    char log_dir[PATH_MAX];
    char *tmp = ".local/share/archivist";
    if (check_path_name_size(log_dir, sizeof(log_dir), home, tmp) == -1)
    {
        return NULL;
    }

    char *created_dir = get_valid_destination(log_dir);
    if (!created_dir)
    {
        return NULL;
    }

    static char full_path[PATH_MAX];
    const char *file_name = "archivist.log";
    if (check_path_name_size(full_path, sizeof(full_path), created_dir, file_name) == -1)
    {
        return NULL;
    }

    free(created_dir);
    return full_path;
}

// Parses through CLI arguments for 'log' functionality
static LogOptions parse_log_opts(int argc, char **argv, int opt_start)
{
    LogOptions opts = {0};

    static struct option long_opts[] =
    {
        {"limit", required_argument, 0, 'l'},
        {"command", required_argument, 0, 'C'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0, long_index = 0;
    char *short_opts = 'l:C:';

    optind = opt_start;
    while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch(opt)
        {
            case 'l':
            {
                opts.limit = parse_int(optarg);
                break;
            }
            case 'C':
            {
                opts.cmd = optarg;
                break;
            }
            case '?':
            {
                break;
            }
        }
    }

    return opts;
}

static int parse_int(char *input)
{
    char *ptr;
    long num = strtol(input, &ptr, 10);
    errno = 0;

    if (errno == EINVAL || errno == ERANGE)
    {
        fprintf(stderr, "Error on strtol(): %s\n", strerror(errno));
        return -1;
    }
    if (num < 0)
    {
        fprintf(stderr, "Size can't be negative: %ld\n", num);
        return -1;
    }
    if (ptr == input)
    {
        fprintf(stderr, "No digits were found!\n");
        return -1;
    }

    return (int)num;
}

static ErrorCode print_log(const char *log_dir, LogOptions opts)
{
    FILE *file = fopen(log_dir, "r");
    if (!file)
    {
        // File doesn't exist
        if (errno == ENOENT)
        {
            printf("No log records available!\n ");
            return EC_SUCCESS;
        }

        // Any other fopen() error
        fprintf(stderr, "Error on fopen(): %s\n", strerror(errno));
        return EC_FOPEN_ERROR;
    }

    // File exists
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    int lines_printed = 0;

    const char *all_cmds[] = {"backup", "delete", "move", "recover", "rename"};
    size_t cmd_len = sizeof(all_cmds) / sizeof(all_cmds[0]);

    while ((read = getline(&line, &len, file)) != -1)
    {
        if (opts.cmd && opts.cmd[0] != '\0')
        {
            // Check if given command is valid and if it's the command of current line
            if (!check_value_in_list(opts.cmd, all_cmds, cmd_len))                
            {
                errno = EINVAL;
                fprintf(stderr, "Invalid command '%s': %s\n", opts.cmd, strerror(errno));
                return EC_INVALID_COMMAND;
            }
            if (!check_cmd_line(opts.cmd))
            {
                continue;
            }
        }

        printf("%s", line);
        lines_printed++;

        // Checks for 'limit' flag
        if(opts.limit > 0 && lines_printed >= opts.limit)
        {
            break;
        }
    }

    free(line);
    fclose(file);

    return EC_SUCCESS;
}

static bool check_cmd_line(const char *cmd)
{
    // Jumps first field
    const char *field = strchr(cmd, '|');
    if (!field)
    {
        return false;
    }

    // Jumps seconds field
    field = strchr(field + 1, '|');
    if (!field)
    {
        return false;
    }

    // Cleans start of "CMD" field
    const char *start = field + 1;
    while (*start == ' ')
    {
        start++;
    }

    // Gets end of "CMD" field
    const char *end = strchr(start, '|');
    if (!end)
    {
        return false;
    }
    // Cleans end of "CMD" field
    while (end > start && *(end + 1) == ' ')
    {
        end--;
    }

    if (start == end)
    {
        return false;
    }

    // Compares given command with line's command
    size_t len = end - start;
    if (strlen(cmd) == len && 
        strncasecmp(start, cmd, len) == 0)
    {
        return true;
    }

    return false;
}

// Writes record in .log file
void log_write(const char *status, const char *cmd, const char *message)
{
    const char *log_path = get_log_path();
    if (!log_path)
    {
        return;
    }

    FILE *log_file = fopen(log_path, "a");
    if (!log_file)
    {
        return;
    }

    char timestamp[32] = {0};
    get_formatted_time(timestamp, sizeof(timestamp));
    if (timestamp[0] == '\0')
    {
        strcpy(timestamp, "N/A");
    }

    fprintf(log_file, "%s | %s | %s | %s\n", timestamp, status, cmd, message);

    fclose(log_file);
}

// Creates log's message before updating the .log file
void update_log_file(const char *status, const char *command, const char *src_path,
                     const char *dst_path, bool error)
{
    char log_msg[512] = {0};
    if (error)
    {
        snprintf(log_msg, sizeof(log_msg), "%s → %s (%s)", src_path, dst_path, strerror(errno));
    }
    else
    {
        snprintf(log_msg, sizeof(log_msg), "%s → %s", src_path, dst_path);
    }

    log_write(status, command, log_msg);
}
