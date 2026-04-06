#define _GNU_SOURCE

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

// Headers
#include "delete.h"
#include "help.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static DeleteOptions parse_delete_options(int argc, char **argv, int opt_start, bool *size_err);
static void delete_element(const char *current_path, DeleteOptions opts, struct dirent *namelist,
                           size_t *dlt_files, size_t *dlt_directories, off_t *dlt_size,
                           Extension *ext, size_t *ext_counter);
static bool delete_directory(DeleteOptions opts, const char *path, size_t *dlt_files,
                             size_t *dlt_directories, off_t *dlt_size);

// Deletes files/subdirectories
int handle_delete(int argc, char **argv)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[2]))
    {
        print_delete_help();
        return 0;
    }

    // Defines starting values
    const char *dir_path = NULL;
    int opt_start = 2;

    // Gets valid base directory
    if (argc >= 3 && argv[2][0] != '-')
    {
        dir_path = argv[2];
        opt_start = 3;
    }

    char *base_dir = get_valid_directory(dir_path);
    if (!base_dir)
    {
        return 4;
    }

    // Parses CLI arguments
    bool size_err = false;
    DeleteOptions opts = parse_delete_options(argc, argv, opt_start, &size_err);
    if (size_err)
    {
        free(base_dir);
        errno = EIO;
        fprintf(stderr, "Invalid size: %s\n", strerror(errno));
        return 9;
    }

    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        free(base_dir);
        perror("scandir");
        return 6;
    }

    Extension *ext = NULL;
    size_t ext_counter = 0;
    if (opts.base.extension && opts.base.extension[0] != '\0')
    {
        ext = get_all_extensions(opts.base.extension, ext_counter);
        if (!ext)
        {
            free(base_dir);
            errno = ENOMEM;
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            return 10;
        }
    }

    size_t dlt_files = 0, dlt_directories = 0;
    off_t dlt_size = 0;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            delete_element(base_dir, opts, namelist[i], &dlt_files, &dlt_directories, &dlt_size, ext, ext_counter);
        }

        free(namelist[i]);
    }

    free(namelist);
    free(base_dir);

    const char *f_out = formatted_output(dlt_size);

    if (opts.base.human_readable)
    {
        printf("Files deleted: %zu\n"
            "Directories deleted: %zu\n"
            "Space freed: %s\n",
            dlt_files, dlt_directories, f_out);
    }
    else
    {
        printf("Files deleted: %zu\n"
            "Directories deleted: %zu\n"
            "Space freed: %jd\n",
            dlt_files, dlt_directories, (intmax_t)dlt_size);
    }

    return 0;
}

// Parses through CLI arguments for 'delete' functionality
static DeleteOptions parse_delete_options(int argc, char **argv, int opt_start, bool *size_err)
{
    DeleteOptions opts = {0};
    opts.base.recursive = true;

    static struct option long_opts[] =
    {
        {"contains", no_argument, 0, 'c'},
        {"dry-run", no_argument, 0, 'd'},
        {"extension", required_argument, 0, 'e'},
        {"human-readable", no_argument, 0, 'h'},
        {"interactive", no_argument, 0, 'i'},
        {"type", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'v'},
        {"recursive", no_argument, 0, 'R'},        
        {"max-size", required_argument, 0, 0},
        {"min-size", required_argument, 0, 0},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "cde:hit:vR";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            // Contains
            case 'c':
            {
                opts.filter.contains = true;
                break;
            }
            // Dry-run
            case 'd':
            {
                opts.action.dry_run = true;
                break;
            }
            // Extension
            case 'e':
            {
                opts.base.extension = optarg;
                break;
            }
            // Human-readable
            case 'h':
            {
                opts.base.human_readable = true;
                break;
            }
            // Interactive
            case 'i':
            {
                opts.action.interactive = true;
                break;
            }
            // Type
            case 't':
            {
                opts.filter.type = optarg;
                break;
            }
            // Verbose
            case 'v':
            {
                opts.action.verbose = true;
                break;
            }
            // Recursive
            case 'R':
            {
                opts.base.recursive = false;
                break;
            }
            // Long arguments
            case 0:
            {
                if (strcasecmp(long_opts[long_index].name, "max-size") == 0)
                {
                    opts.filter.max_size = get_size(optarg);
                }
                else if (strcasecmp(long_opts[long_index].name, "min-size") == 0)
                {
                    opts.filter.min_size = get_size(optarg);
                }
                if (opts.filter.max_size == -1 || opts.filter.min_size == -1)
                {
                    *size_err = true;
                }
                break;
            }
            // Error
            case '?':
            {
                break;
            }
        }
    }
    
    return opts;
}

// Deletes elements
static void delete_element(const char *current_path, DeleteOptions opts, struct dirent *namelist,
                           size_t *dlt_files, size_t *dlt_directories, off_t *dlt_size,
                           Extension *ext, size_t *ext_counter)
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

    if (S_ISDIR(st.st_mode))
    {
        // Checks for directory type
        if (is_directory_type(opts.filter.type))
        {
            bool can_nuke = true;

            if (opts.filter.contains && opts.filter.contains[0] != '\0')
            {
                if (!match_name(opts.filter.contains, namelist->d_name))
                {
                    can_nuke = false;
                }
            }

            if (opts.filter.max_size || opts.filter.min_size)
            {
                off_t dir_size = 0;
                if (!match_directory_size(new_path, opts.filter.max_size, opts.filter.min_size, &dir_size))
                {
                    can_nuke = false;
                }
            }

            // All directory can be deleted
            if (can_nuke)
            {
                if (delete_directory(opts, new_path, dlt_files, dlt_directories, dlt_size))
                {
                    (*dlt_directories)++;
                }

                return;
            }
        }

        // recursivelly calls directory's elements
        if (opts.base.recursive)
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

        if (opts.action.dry_run)
        {
            printf("[DRY-RUN] Would remove directory %s\n", new_path);
            (*dlt_directories)++;
        }
        else
        {
            if (rmdir(new_path) == 0)
            {
                (*dlt_directories)++;

                if (opts.action.verbose)
                {
                    printf("Directory deleted %s\n", new_path);
                }
            }
        }
    }

    else
    {
        // Checks for element's type
        if (opts.filter.type && !match_type(opts.filter.type, st.st_mode))
        {
            return;
        }

        // Checks for matching extension
        if (ext && !match_extension(ext, ext_counter, namelist->d_name))
        {
            return;
        }

        // Checks for size
        if ((opts.filter.max_size || opts.filter.min_size) &&
            !match_size(opts.filter.max_size, opts.filter.min_size, st.st_size))
        {
            return;
        }

        bool success = false;

        if (opts.action.dry_run)
        {
            printf("[DRY-RUN] Would delete file %s\n", new_path);
            success = true;
        }
        else
        {
            if (remove(new_path) == 0)
            {
                if (opts.action.verbose)
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

/*
interactive
*/

// Fully deletes a directory
static bool delete_directory(DeleteOptions opts, const char *path, size_t *dlt_files,
                             size_t *dlt_directories, off_t *dlt_size)
{
    struct dirent **ptr;
    int n = scandir(path, &ptr, NULL, alphasort);
    if (n == -1)
    {
        return false;
    }

    bool deleted = false;
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

        if (S_ISDIR(st.st_mode))
        {
            if (delete_directory(opts, new_path, dlt_files, dlt_directories, dlt_size))
            {
                deleted = true;
            }

            bool success = false;
            if (opts.action.dry_run)
            {
                printf("[DRY-RUN] Woud remove directory %s\n", new_path);
                success = true;
            }
            else
            {
                if (rmdir(new_path) == 0)
                {
                    if (opts.action.verbose)
                    {
                        printf("Directory deleted: %s", new_path);
                    }
                    success = true;
                }
            }

            if (success)
            {
                (*dlt_directories)++;
                deleted = true;
            }
        }

        else
        {
            bool success = false;
            if (opts.action.dry_run)
            {
                printf("[DRY-RUN] Would delete file %s\n", new_path);
                success = true;
            }
            else
            {
                if (remove(new_path) == 0)
                {
                    if (opts.action.verbose)
                    {
                        printf("File deleted: %s", new_path);
                    }
                    success = true;
                }
            }

            if (success)
            {
                deleted = true;
                (*dlt_files)++;
                (*dlt_size) += st.st_size;
            }
        }

        free(ptr[i]);
    }

    free(ptr);

    bool success = false;

    if (opts.action.dry_run)
    {
        printf("[DRY-RUN] Would delete dyrectory %s\n");
        success = true;
    }

    // Tries to delete current directory
    if (rmdir(path) == 0)
    {
        if (opts.action.verbose)
        {
            printf("Directory deleted: %s\n", path);
        }
        success = true;
    }

    if (success)
    {
        (*dlt_directories)++;
        deleted = true;
    }

    return deleted;
}
