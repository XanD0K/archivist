#ifndef UTILS_FILTER_H
#define UTILS_FILTER_H

// Libraris
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

// Prototypes
bool match_type(const char *type, mode_t mode);
bool match_size(off_t max_size, off_t min_size, off_t size);

#endif