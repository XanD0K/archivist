#define _GNU_SOURCE

// Libraries
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>  // O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>  // open() | close() | read()

// Headers
#include "cli_opts.h"
#include "utils.h"

// Globals (CRC32)
static uint32_t crc32_table[256];
static bool table_initialized = false;

// Prototypes
static void init_crc32_table(void);

// Checks for valid directory, setting (.) as default, and removing trailing "/"
char *get_valid_directory(const char *path)
{
    const char *p = (!path) ? "." : path;
    char *base_dir = strdup(p);
    if (!base_dir)
    {
        fprintf(stderr, "Error on strdup(): %s\n", strerror(errno));
        return NULL;
    }

    struct stat st;
    // Tries to fill st with directory's data
    if (lstat(base_dir, &st) != 0)
    {
        fprintf(stderr, "Error in stat(): %s\n", strerror(errno));
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
        fprintf(stderr, "Error on strdup(): %s\n", strerror(errno));
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
    while (token)
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

// Filters "." and ".." to prevent infinity loop
int scandir_no_dot_filter(const struct dirent *entry)
{
    return (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0);
}

// Filters any element that starts with "." to prevent showing hidden content
int scandir_visible_only(const struct dirent *entry)
{
    return (entry->d_name[0] != '.');
}


int scandir_show_hidden_files(const struct dirent *entry)
{
    if (entry->d_name[0] == '.')
    {
        if (entry->d_type == DT_DIR)
        {
            return 0;
        }

        return 1;
    }

    return 1;
}

// Prints a more readable output message
char *formatted_output(off_t total_size)
{
    static const char *sizes[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    const float buffer = 1024.0f;

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
    const char *ext = (dot) ? dot + 1 : "";
    return ext;
}

// Retrieves user's selected extensions (-e flag)   
Extension *get_all_extensions(const char *exts, size_t *ext_counter)
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
    while (token)
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
bool check_help(int argc, const char *argv)
{
    if (argc == 3)
    {
        if (strcasecmp(argv, "--help") == 0 ||
            strcasecmp(argv, "help") == 0)
            {
                return true;
            }
    }

    return false;
}

// Gets directory's suffix
const char *get_suffix(const char *path, const char *base_dir)
{
    const char *suffix = path + strlen(base_dir);
    if (*suffix == '/')
    {
        suffix++;
    }

    return suffix;
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
bool file_needs_backup(struct stat *st_src, const char *src_dir, const char *dst_dir)
{
    // File doesn't exist on destiny directory
    struct stat st_dst;
    if (lstat(dst_dir, &st_dst) != 0)
    {
        return true;
    }

    // Compares files' size
    if (st_src->st_size != st_dst.st_size)
    {
        return true;
    }

    // Compares files's hash
    uint32_t hash_src = quick_file_hash(src_dir);
    uint32_t hash_dst = quick_file_hash(dst_dir);

    if (hash_src != hash_dst)
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
    
    // Fallback
    if (ret < 0)
    {
        char buffer[chunk];
        ssize_t bytes;

        while ((bytes = read(input, buffer, sizeof(buffer))) > 0)
        {
            if (write(output, buffer, (size_t)bytes) != bytes)
            {
                close(input);
                close(output);
                return -1;
            }
        }
        ret = (bytes == 0) ? 0 : -1;
    }

    close(input);
    close(output);
    return (ret == 0) ? 0 : -1;
}

// Checks if given value is in a list of values
bool check_value_in_list(const char *name, const char *list[], size_t len)
{
    bool found = false;
    for (size_t i = 0; i < len; i++)
    {
        // Invalid sort method defaults to "name"
        if(strcasecmp(name, list[i]) == 0)
        {
            found = true;
            break;
        }
    }

    return found;
}

// Gets current time
void get_formatted_time(char *buffer, size_t buffer_len)
{
    time_t now = time(NULL);  // Current time in seconds (since 1970)
    struct tm *tm_info = localtime(&now);  // Converts to local format
    strftime(buffer, buffer_len, "%F %T", tm_info);
}

// Entirely frees a dirent struct
void free_dirent(struct dirent **tmp,int len)
{
    for (int i = 0; i < len; i++)
    {
        free(tmp[i]);
    }

    free(tmp);
}

// Validates 'type' flag
char *validate_type(const char *type)
{
    if (!type || type[0] == '\0')
    {
        return NULL;
    }

    if (is_directory_type(type) ||
        is_slink_type(type) ||
        is_file_type(type))
    {
        return (char *)type;
    }

    return NULL;
}

bool is_directory_type (const char *type)
{
    if (!type || type[0] == '\0')
    {
        return false;
    }

    return (strcasecmp(type, "d") == 0 ||
            strcasecmp(type, "dir") == 0 ||
            strcasecmp(type, "directory") == 0);
}

bool is_slink_type (const char *type)
{
    if (!type || type[0] == '\0')
    {
        return false;
    }

    return (strcasecmp(type, "sl") == 0 ||
            strcasecmp(type, "slink") == 0 ||
            strcasecmp(type, "symbolic-link") == 0);
}

bool is_file_type (const char *type)
{
    if (!type || type[0] == '\0')
    {
        return false;
    }

    return (strcasecmp(type, "f") == 0 ||
            strcasecmp(type, "file") == 0);
}

// Populates the crc32_table with every possible value for 8 bits
static void init_crc32_table(void)
{
    // For each possible byte value (0 to 255)
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        // For every bit of current byte
        for (int j = 0; j < 8; j++)
        {
            int bit = crc & 1;                       // Gets least significant bit (right one)
            uint32_t mask = (uint32_t)(-bit);        // 0xFFFFFFFF if bit == 1, else 0
            crc = (crc >> 1) ^ (0xEDB88320 & mask);  // if bit == 1, XOR with CRC32 polynomial, else shift right
        }
        crc32_table[i] = crc;
    }
}

// Gets hash value for given chunk of data
uint32_t crc32(const void *data, size_t len)
{
    // Initializes table on first run
    if(!table_initialized)
    {
        init_crc32_table();
        table_initialized = true;
    }

    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *buf = (const uint8_t *)data;

    for (size_t i = 0; i < len; i++)
    {
        // Finds value on table for each byte
        crc = crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }

    // Bitwise inversion (standard CRC32)
    return crc ^ 0xFFFFFFFF;
}

// Calculates hash value for given file path
uint32_t quick_file_hash(const char *path)
{
    int file = open(path, O_RDONLY);
    if (file < 0)
    {
        return 0;
    }

    // Reads 64KB of data
    const size_t MAX = 64 * 1024;
    uint8_t buffer[MAX];

    ssize_t bytes = read(file, buffer, sizeof(buffer));
    close(file);

    if (bytes <= 0)
    {
        return 0;
    }

    // Returns CRC32 of the data
    return crc32(buffer, (size_t)bytes);
}
