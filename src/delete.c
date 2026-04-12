#define _GNU_SOURCE

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "cli_parse_common.h"
#include "commands.h"
#include "delete.h"
#include "help.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void delete_element(const char *current_path, DeleteOptions *opts, struct dirent *namelist,
                           size_t *dlt_files, size_t *dlt_directories, off_t *dlt_size,
                           Extension *ext, size_t ext_counter);
static void delete_directory(DeleteOptions *opts, const char *path, size_t *dlt_files,
                             size_t *dlt_directories, off_t *dlt_size);

// Setup logic for 'delete' feature
int handle_delete(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_delete_help,
                                            parse_delete_options, sizeof(DeleteOptions));
    if (!context)
    {
        return 10;
    }
    if (context->error_code != 0)
    {
        free_command_context(context);
        return (context->error_code == -1) ? 0 : context->error_code;
    }

    DeleteOptions *opts = (DeleteOptions*)context->opts;

    // Retrieves user's typed extensions
    Extension *ext = NULL;
    size_t ext_counter = 0;
    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        ext = get_all_extensions(opts->filter.extension, &ext_counter);
        if (!ext)
        {
            free_command_context(context);
            errno = ENOMEM;
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            return 10;
        }
    }

    // Retrieves directory's content
    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        free_extensions(ext, ext_counter);
        free_command_context(context);
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        return 6;
    }

    // Parses directory's content
    size_t dlt_files = 0, dlt_directories = 0;
    off_t dlt_size = 0;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            delete_element(context->base_dir, opts, namelist[i], &dlt_files, &dlt_directories, &dlt_size, ext, ext_counter);
        }

        free(namelist[i]);
    }

    free(namelist);
    free_extensions(ext, ext_counter);

    // Output message
    char *f_out = formatted_output(dlt_size);
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Files deleted: %zu\n"
               "[DRY-RUN] Directories deleted: %zu\n"
               "[DRY-RUN] Space freed: %s\n",
               dlt_files, dlt_directories, f_out);
    }
    else
    {
        printf("Files deleted: %zu\n"
               "Directories deleted: %zu\n",
               dlt_files, dlt_directories);

        if (opts->base.human_readable)
        {
            printf("Space freed: %s\n", f_out);
        }
        else
        {
            printf("Space freed: %jd\n", (intmax_t)dlt_size);
        }
    }

    free_command_context(context);
    free(f_out);
    return 0;
}

// Parses through CLI arguments for 'delete' functionality
int parse_delete_options(int argc, char **argv, int opt_start, void *opts_out)
{
    DeleteOptions *opts = (DeleteOptions*)opts_out;

    int ret;
    ret = parse_common_opts(argc, argv, opt_start, &opts->base);
    if (ret != 0)
    {
        return ret;
    }
    ret = parse_filter_options(argc, argv, opt_start, &opts->filter);
    if (ret != 0)
    {
        if (ret == PARSE_ERROR_INVALID_SIZE)
        {
            errno = EIO;
            fprintf(stderr, "Invalid size: %s\n", strerror(errno));
            return 9;
        }
        return ret;
    }
    ret = parse_action_options(argc, argv, opt_start, &opts->action);
    if (ret != 0)
    {
        return ret;
    }
    
    return 0;
}

// Deletes elements
static void delete_element(const char *current_path, DeleteOptions *opts, struct dirent *namelist,
                           size_t *dlt_files, size_t *dlt_directories, off_t *dlt_size,
                           Extension *ext, size_t ext_counter)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    struct stat st;
    char new_path[PATH_MAX];
    if (check_path_name_size(new_path, sizeof(new_path), current_path, namelist->d_name) == -1)
    {
        return;
    }

    if (stat(new_path, &st) != 0)
    {
        return;
    }

    // === DIRECTORIES ===
    if (S_ISDIR(st.st_mode))
    {
        // Checks for directory type
        if (is_directory_type(opts->filter.type))
        {
            bool can_nuke = true;

            // Checks for 'contains' filter
            if (opts->filter.contains && opts->filter.contains[0] != '\0')
            {
                if (!match_name(opts->filter.contains, namelist->d_name))
                {
                    can_nuke = false;
                }
            }

            // Checks for 'max-size' and 'min-size' filters
            if (opts->filter.max_size || opts->filter.min_size)
            {
                off_t dir_size = 0;
                if (!match_directory_size(new_path, opts->filter.max_size, opts->filter.min_size, &dir_size))
                {
                    can_nuke = false;
                }
            }

            // Deletes whole directory
            if (can_nuke)
            {
                delete_directory(opts, new_path, dlt_files, dlt_directories, dlt_size);
                return;
            }
        }

        // Recursivelly calls directory's elements
        if (opts->base.recursive)
        {
            struct dirent **entry;
            int n = scandir(new_path, &entry, NULL, alphasort);
            if (n == -1)
            {
                return;
            }

            for (int i = 0; i < n; i++)
            {
                delete_element(new_path, opts, entry[i], dlt_files, dlt_directories, dlt_size, ext, ext_counter);
                free(entry[i]);
            }

            free(entry);
        }

        // Tries to remove current directory (if empty after recursion)
        if (opts->action.dry_run)
        {
            printf("[DRY-RUN] Would remove directory %s\n", new_path);
            (*dlt_directories)++;
        }
        else
        {
            if (opts->action.interactive)
            {
                char *prompt = NULL;
                if (asprintf(&prompt, "Remove directory: '%s'?", new_path) == -1)
                {
                    fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                    return;
                }
                if (!get_answer(prompt))
                {
                    free(prompt);
                    return;
                }

                free(prompt);
            }

            // Tries to remove current directory
            if (rmdir(new_path) == 0)
            {
                (*dlt_directories)++;
                if (opts->action.verbose)
                {
                    printf("Directory deleted %s\n", new_path);
                }
            }
        }
    }

    // === FILES ===
    else
    {
        // Checks for element's type
        if (opts->filter.type && !match_type(opts->filter.type, st.st_mode))
        {
            return;
        }

        // Checks for matching extension
        if (ext && !match_extension(ext, ext_counter, namelist->d_name))
        {
            return;
        }

        // Checks for size
        if ((opts->filter.max_size || opts->filter.min_size) &&
            !match_size(opts->filter.max_size, opts->filter.min_size, st.st_size))
        {
            return;
        }

        bool success = false;
        if (opts->action.dry_run)
        {
            printf("[DRY-RUN] Would delete file %s\n", new_path);
            success = true;
        }
        else
        {
            if (opts->action.interactive)
            {
                char *prompt = NULL;
                if (asprintf(&prompt, "Delete file: '%s'? ", new_path) == -1)
                {
                    fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                    free(prompt);
                    return;
                }

                free(prompt);
            }

            if (remove(new_path) == 0)
            {
                if (opts->action.verbose)
                {
                    printf("File deleted: %s\n", new_path);
                }
                success = true;
            }
        }

        if (success)
        {
            (*dlt_files)++;
            (*dlt_size) += st.st_size;
        }
    }
}

// Fully deletes a directory
static void delete_directory(DeleteOptions *opts, const char *path, size_t *dlt_files,
                             size_t *dlt_directories, off_t *dlt_size)
{
    struct dirent **ptr;
    int n = scandir(path, &ptr, NULL, alphasort);
    if (n == -1)
    {
        return;
    }

    for (int i = 0; i < n; i ++)
    {
        if (strcmp(ptr[i]->d_name, ".") == 0 || strcmp(ptr[i]->d_name, "..") == 0)
        {
            free(ptr[i]);
            continue;
        }

        char new_path[PATH_MAX];
        if (check_path_name_size(new_path, sizeof(new_path), path, ptr[i]->d_name) == -1)
        {
            free(ptr[i]);
            continue;
        }

        struct stat st;
        if (stat(new_path, &st) != 0)
        {
            free(ptr[i]);
            continue;
        }

        // === DIRECTORY ===
        if (S_ISDIR(st.st_mode))
        {
            // Recursive call for deletion
            delete_directory(opts, new_path, dlt_files, dlt_directories, dlt_size);
        }

        // === FILE ===
        else
        {
            bool success = false;
            if (opts->action.dry_run)
            {
                printf("[DRY-RUN] Would delete file %s\n", new_path);
                success = true;
            }
            else
            {
                if (opts->action.interactive)
                {
                    char *prompt = NULL;
                    if (asprintf(&prompt, "Delete file: '%s'? ", new_path) == -1)
                    {
                        fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                        free(ptr[i]);
                        free(prompt);
                        continue;
                    }

                    free(prompt);
                }
                if (remove(new_path) == 0)
                {
                    if (opts->action.verbose)
                    {
                        printf("File deleted: %s\n", new_path);
                    }
                    success = true;
                }
            }

            if (success)
            {
                (*dlt_files)++;
                (*dlt_size) += st.st_size;
            }
        }

        free(ptr[i]);
    }

    free(ptr);

    // Removes current directory
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Would delete directory %s\n", path);
        (*dlt_directories)++;
    }
    else
    {
        if (opts->action.interactive)
        {
            char *prompt = NULL;
            if (asprintf(&prompt, "Remove directory: '%s'?", path) == -1)
            {
                fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                free(prompt);
                return;
            }

            free(prompt);
        }

        // Tries to delete current directory
        if (rmdir(path) == 0)
        {
            if (opts->action.verbose)
            {
                printf("Directory deleted: %s\n", path);
            }
            (*dlt_directories)++;
        }
    }
}
