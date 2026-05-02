#ifndef UTILS_FILTER_H
#define UTILS_FILTER_H

// Libraries
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>  // size_t
#include <sys/types.h>  // off_t

// Headers
#include "commands.h"
#include "cli_opts.h"

// Prototypes
bool check_directory_flags(const struct dirent *namelist, const char *full_path,
                           const FilterOptions *filter, ScandirFilter scandir_filter);
bool check_file_flags(const struct dirent *namelist, Extension *ext, const char *full_path,
                      const FilterOptions *filter, size_t ext_counter, size_t *err_counter);
bool match_name(const char *name, const char *contains);
bool match_type(const char *type, mode_t mode);
bool match_size(off_t max_size, off_t min_size, off_t size);
bool match_directory_size(const char *path, off_t max_size, off_t min_size,
                          off_t *total_size, ScandirFilter filter);
bool match_extension(Extension *exts, size_t ext_counter, const char *name);

#endif
