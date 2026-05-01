#ifndef LOG_H
#define LOG_H

// Library
#include <stdbool.h>

// Headers
#include "error_code.h"

// Constants
#define LOG_SUCCESS "SUCCESS"
#define LOG_ERROR "ERROR"
#define CMD_BACKUP "BACKUP"
#define CMD_DELETE "DELETE"
#define CMD_MOVE "MOVE"
#define CMD_RECOVER "RECOVER"
#define CMD_RENAME "RENAME"

// Structure to all CLI flags of 'log' functionality
typedef struct
{
    char *cmd;
    int limit;
    bool erase;
    ErrorCode err;
} LogOptions;

// Prototypes
ErrorCode handle_log(int argc, char **argv, int min_args);
void log_write(const char *status, const char *cmd, const char *message);
void update_log_file(const char *status, const char *command, const char *src_path,
                     const char *dst_path, bool error);

#endif
