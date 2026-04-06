#ifndef UTILS_FILTER_H
#define UTILS_FILTER_H

// Libraris
#include <stdbool.h>
#include <stddef.h>  // size_t
#include <sys/types.h>  // off_t

// Prototypes
bool match_name(char *contains, const char *name);
bool match_type(const char *type, mode_t mode);
bool is_directory_type (const char *type);
bool match_size(off_t max_size, off_t min_size, off_t size);
bool match_directory_size(const char *path, off_t max_size, off_t min_size , off_t *total_size);
bool match_extension(Extension *exts, size_t ext_counter, char *name);

#endif
