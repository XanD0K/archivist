#ifndef UTILS_H
#define UTILS_H

// Libraries

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>  // size_t
#include <sys/stat.h>
#include <sys/types.h>  // off_t

// Structure for each extension
typedef struct
{
    char *extension;
    size_t file_count;
    off_t size;
} Extension;

// Prototypes
char *get_valid_directory(const char *path);
char *get_valid_destination(const char *path);
int scandir_no_dot_filter(const struct dirent *entry);
char *formatted_output(off_t total_size);
const char *get_clean_extension(const char *name);
Extension *get_all_extensions(char *exts, size_t *ext_count);
void free_extensions(Extension *ext, size_t ext_counter);
bool check_help(int argc, char *argv);
off_t get_size(char *size);
const char *get_suffix(char newpath[], const char *base_dir);
bool get_answer(const char *prompt);
int check_path_name_size(char *dst, size_t len, const char *prefix, const char *suffix);
bool file_needs_backup(struct stat *st_src, const char *dst_dir);
int copy_file(const char *src_path, const char *dst_path);
bool check_value_in_list(char *name, const char *list[], size_t len);
void get_formatted_time(char *timestamp, size_t len);
void free_dirent(struct dirent **tmp,int len);

#endif
