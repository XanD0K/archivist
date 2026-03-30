#define _GNU_SOURCE

// Libraries
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// Headers
#include "utils.h"

// Checks for valid directory, setting (.) as default, and removing trailing /
char *get_valid_directory(const char *path)
{
    const char *p = (!path) ? "." : path;
    char *base_dir = strdup(p);
    if (!base_dir)
    {
        return NULL;
    }

    struct stat st;
    // Tries to fill st with directory's data
    if (stat(base_dir, &st) != 0)
    {
        fprintf(stderr, "Error in stat() for %s: %s\n", p, strerror(errno));
        free(base_dir);
        return NULL;
    }

    // Checks if path is a directory
    if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        fprintf(stderr, "Error accessing diretory %s: %s\n", p, strerror(errno));
        free(base_dir);
        return NULL;
    }

    // Ensures path has no trailing /
    size_t len = strlen(base_dir);
    if (len > 0 && base_dir[len - 1] == '/')
    {
        base_dir[len - 1] = '\0';
    }

    return base_dir;
}

// Prints a more readable output message
char *formatted_output(off_t total_size)
{
    static const char *sizes[] = {"bytes", "kilobytes", "megabytes", "gigabytes", "terabytes", "petabytes"};
    const float buffer = 1024.0;

    float result = (float)total_size;
    size_t index = 0;    
    
    while (result >= buffer)
    {
        result /= buffer;
        index++;
    }

    // Limits index to 5
    index = (index < 5) ? index : 5;

    char *str;
    asprintf(&str, "%.2f %s", result, sizes[index]);
    return str;
}

// Gets extension without .
const char *get_extension(const char *name)
{
    const char *dot = strrchr(name, '.');
    const char *ext = (dot != NULL) ? dot + 1 : "";
    return ext;
}

// Checks for help flag
bool check_help(int argc, char *argv)
{
    if (argc == 3)
    {
        if (strcasecmp(argv, "-h") == 0 ||
            strcasecmp(argv, "--help") == 0 ||
            strcasecmp(argv, "help") == 0)
            {
                return true;
            }
    }

    return false;
}

// Checks for valid sort method
bool check_sort(char *sort, const char **sorts, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (strcasecmp(sort, sorts[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

off_t get_size(char *size)
{
    const off_t BUFFER = 1024;
    char *ptr;
    long num = strtol(size, &ptr, 10);
    errno = 0;

    if (errno == EINVAL || errno == ERANGE)
    {
        perror("strtol");
        return -1;
    }
    if (num < 0)
    {
        fprintf(stderr, "Size can't be negative: %ld\n", num);
        return -1;
    }
    if (ptr == size)
    {
        fprintf(stderr, "No digits were found\n");
        return -1;
    }

    num = (off_t)num;

    if(ptr == NULL || strcasecmp(ptr, "B") == 0)
    {
        return num;
    }
    else if (strcasecmp(ptr, "K") == 0 || strcasecmp(ptr, "KB") == 0)
    {
        return num * BUFFER;
    }
    else if (strcasecmp(ptr, "M") == 0 || strcasecmp(ptr, "MB") == 0)
    {
        return num * BUFFER * BUFFER;
    }
    else if (strcasecmp(ptr, "G") == 0 || strcasecmp(ptr, "GB") == 0)
    {
        return num * BUFFER * BUFFER * BUFFER;
    }
    else if (strcasecmp(ptr, "T") == 0 || strcasecmp(ptr, "TB") == 0)
    {
        return num * BUFFER * BUFFER * BUFFER * BUFFER;
    }

    return -1;
}