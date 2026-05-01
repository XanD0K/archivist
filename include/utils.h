#ifndef UTILS_H
#define UTILS_H

// Libraries
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>  // size_t
#include <stdint.h>  // uint8_t | uint32_t
#include <sys/stat.h>
#include <sys/types.h>  // off_t

// Type/Alias
typedef int (*ScandirFilter)(const struct dirent *);

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
int scandir_visible_only(const struct dirent *entry);
int scandir_show_hidden_files(const struct dirent *entry);
char *formatted_output(off_t total_size);
const char *get_clean_extension(const char *name);
Extension *get_all_extensions(const char *exts, size_t *ext_count);
void free_extensions(Extension *ext, size_t ext_counter);
bool check_help(int argc, const char *argv);
const char *get_suffix(const char *path, const char *base_dir);
bool get_answer(const char *prompt);
int check_path_name_size(char *dst, size_t len, const char *prefix, const char *suffix);
bool file_needs_backup(struct stat *st_src, const char *src_dir, const char *dst_dir);
int copy_file(const char *src_path, const char *dst_path);
bool check_value_in_list(const char *name, const char *list[], size_t len);
void get_formatted_time(char *timestamp, size_t len);
void free_dirent(struct dirent **tmp,int len);
char *validate_type(const char *type);
bool is_directory_type (const char *type);
bool is_slink_type (const char *type);
bool is_file_type (const char *type);
uint32_t crc32(const void *data, size_t len);
uint32_t quick_file_hash(const char *path);

#endif
