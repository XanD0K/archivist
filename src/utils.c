#define _GNU_SOURCE

// Libraries
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// Headers
#include "cli_opts.h"
#include "utils.h"

// Checks for valid directory, setting (.) as default, and removing trailing "/"
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
    if (len > 1 && base_dir[len - 1] == '/')
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

// Gets extension without "."
const char *get_clean_extension(const char *name)
{
    const char *dot = strrchr(name, '.');
    const char *ext = (dot != NULL) ? dot + 1 : "";
    return ext;
}

// Retrieves user's selected extensions (-e flag)   
Extension *get_all_extensions(char *exts, size_t *ext_counter)
{
    if (exts == NULL || exts[0] == '\0')
    {
        return NULL;
    }

    *ext_counter = 0;

    Extension *output_ext = NULL;
    char *exts_cpy = strdup(exts);
    if (!exts_cpy)
    {
        return NULL;
    }

    char *token = strtok(exts_cpy, ",");
    while (token != NULL)
    {
        Extension *tmp = realloc(output_ext, (*ext_counter + 1) * sizeof(Extension));
        if (tmp == NULL)
        {
            goto cleanup;
        }

        output_ext = tmp;
        
        output_ext[*ext_counter].extension = strdup(token);
        if (!output_ext[*ext_counter].extension)
        {
            goto cleanup;
        }

        // Sets extension to lowercase
        for (char *p = output_ext[*ext_counter].extension; *p; p++)
        {
            *p = tolower((unsigned char)*p);
        }

        output_ext[*ext_counter].file_count = 0;
        output_ext[*ext_counter].size = 0;
        (*ext_counter)++;

        token = strtok(NULL, ",");
    }

    free(exts_cpy);
    return output_ext;

cleanup:
    for (size_t i = 0; i < *ext_counter; i++)
    {
        free(output_ext[i].extension);
    }
    free(exts_cpy);
    free(output_ext);
    return NULL;
}

// Frees array of Extension structures, and each of their 'extension' field
void free_extensions(Extension *ext, size_t ext_counter)
{
    if (!ext)
    {
        return;
    }

    for (int i = 0; i < ext_counter; i++)
    {
        free(ext[i].extension);
    }

    free(ext);
}

// Checks for 'help' flag
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

// Converts user's input to off_t size
off_t get_size(char *size)
{
    const off_t BUFFER = 1024;
    char *ptr;
    long num = strtol(size, &ptr, 10);
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
    if (ptr == size)
    {
        fprintf(stderr, "No digits were found!\n");
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

// Gets directory's suffix
const char *get_suffix(char newpath[], const char *base_dir)
{
    const char *suffix = newpath + strlen(base_dir);

    return (*suffix == '/') ? suffix++ : suffix;
}

// Gets user's input
bool get_answer(const char *prompt)
{
    while (1)
    {
        printf("%s [y/n]: ", prompt);
        fflush(stdout);

        char *input = NULL;
        size_t len = 0;

        ssize_t nread = getline(&input, &len, stdin);
        if (nread == -1)
        {
            free(input);
            return false;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcasecmp(input, "YES") == 0 || strcasecmp(input, "Y") == 0)
        {
            free(input);
            return true;
        }
        
        if (strcasecmp(input, "NO") == 0 || strcasecmp(input, "N") == 0)
        {
            free(input);
            return false;
        }

        free(input);
        printf("Invalid answer! Say YES (Y) or NO (N)\n");
    }
}

// Checks if directory will overflow maximum size
int check_path_name_size(char *dst, size_t len, const char *prefix, const char *suffix)
{
    int ret = (suffix && suffix[0])
        ? snprintf(dst, len, "%s/%s", prefix, suffix)
        : snprintf(dst, len, "%s", prefix);

    if (ret < 0 || (size_t)ret >= len)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}
