#ifndef UTILS_H
#define UTILS_H

// Libraries

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>  // size_t
#include <sys/types.h>  // off_t

// Structure for each extension
typedef struct Extension
{
    char *extension;
    size_t file_count;
    off_t size;
} Extension;

// Prototypes
char *get_valid_directory(const char *path);
char *formatted_output(off_t total_size);
const char *get_clean_extension(const char *name);
Extension *get_all_extensions(char *exts, size_t *ext_count);
void free_extensions(Extension *ext, size_t ext_counter);
bool check_help(int argc, char *argv);
off_t get_size(char *size);
const char *get_suffix(char newpath[], const char *base_dir);
bool get_answer(const char *prompt);
int check_path_name_size(char *dst, size_t len, const char *prefix, const char *suffix);

#endif
