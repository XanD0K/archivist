#define _GNU_SOURCE

// Libraries
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>  // open()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>  // close()

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

// Creates destination directory
char *get_valid_destination(const char *path)
{
    // Checks for valid directory
    if (!path || path[0] == '\0')
    {
        errno = ENOTDIR;
        fprintf(stderr, "Error accessing diretory %s: %s\n", path, strerror(errno));
        return NULL;
    }

    // Copies original path
    char *cpy_path = strdup(path);
    if (!cpy_path)
    {
        fprintf(stderr, "Error duplicating diretory %s: %s\n", path, strerror(errno));
        return NULL;
    }

    // Creates starting path
    char current_path[PATH_MAX];
    current_path[0] = '.';
    current_path[1] = '\0';
    if (cpy_path[0] == '/')
    {
        current_path[0] = '/';
    }

    // Iterates through every directory
    char *token = strtok(cpy_path, "/");
    while (token != NULL)
    {
        // Creates new path
        char new_path[PATH_MAX];
        if (check_path_name_size(new_path, sizeof(new_path), current_path, token) == -1)
        {
            free(cpy_path);
            fprintf(stderr, "Path too long: %s\n", strerror(errno));
            return NULL;
        }

        // Creates directory
        if (mkdir(new_path, 0755) != 0)
        {
            if (errno != EEXIST)
            {
                free(cpy_path);
                fprintf(stderr, "Error on mkdir(): %s\n", strerror(errno));
                return NULL;
            }
        }

        // Updates current path for recursiveness
        if (check_path_name_size(current_path, sizeof(current_path), new_path, NULL) == -1)
        {
            free(cpy_path);
            fprintf(stderr, "Path too long: %s\n", strerror(errno));
            return NULL;
        }
        token = strtok(NULL, "/");
    }

    free(cpy_path);
    return strdup(current_path);
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
            *p = (char)tolower((unsigned char)*p);
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

    for (size_t i = 0; i < ext_counter; i++)
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

// Compares source and destiny files
bool file_needs_backup(struct stat *st_src, const char *dst_dir)
{
    // File doesn't exist on destiny directory
    struct stat st_dst;
    if (stat(dst_dir, &st_dst) != 0)
    {
        return true;
    }

    // Checks for changes in file (mtim | size)
    if (st_src->st_mtim.tv_sec != st_dst.st_mtim.tv_sec ||  // Compares seconds
        st_src->st_mtim.tv_nsec != st_dst.st_mtim.tv_nsec)  // Compares nanoseconds
    {
        return true;
    }

    if (st_src->st_size != st_dst.st_size)
    {
        return true;
    }

    return false;
}

// Copies file from one directory to another
int copy_file(const char *src_path, const char *dst_path)
{

    int input = open(src_path, O_RDONLY);
    if (input < 0)
    {
        return -1;
    }

    int output = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output < 0)
    {
        close(input);
        return -1;
    }

    off_t offset = 0;
    ssize_t ret;
    const size_t chunk = 1UL << 21;

    while ((ret = copy_file_range(input, &offset, output, NULL, chunk, 0)) > 0)
    {
        // Loop keeps running while there is data to be copied
    }

    close(input);
    close(output);

    return (ret == 0) ? 0 : -1;
}
