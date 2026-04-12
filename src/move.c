#define _GNU_SOURCE

// Libraries
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
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
#include "help.h"
#include "move.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static char *get_valid_destination(const char *path);
static void move_element(char *current_path, char *dst_dir, MoveOptions *opts,
                         struct dirent *namelist, Extension *ext, size_t ext_counter,
                         size_t *moved_files, size_t *created_directories);
static char *match_existed_file(char *dst_dir, char *new_dst_dir, char *name, bool skip, bool force);

// Setup logic for 'move' feature
int handle_move(int argc, char **argv, int min_args)
{
    CommandContext *context = setup_command(argc, argv, min_args, print_move_help,
                                            parse_move_opts, sizeof(MoveOptions));
    if (!context)
    {
        return 10;
    }
    if (context->error_code != 0)
    {
        free_command_context(context);
        return (context->error_code == -1) ? 0 : context->error_code;
    }

    MoveOptions *opts = (MoveOptions*)context->opts;

    const char *dir_path_dst = (argc >= 4 && argv[3][0] != '-') ? argv[2] : argv[3];
    // Checks/Creates destination directory
    char *dst_dir = get_valid_destination(dir_path_dst);
    if (!dst_dir)
    {
        return 4;
    }

    // Retrieves user's selected extensions (-e|--extension flag)
    Extension *ext = NULL;
    size_t ext_counter = 0;
    if (opts->filter.extension && opts->filter.extension[0] != '\0')
    {
        ext = get_all_extensions(opts->filter.extension, &ext_counter);
        if (!ext)
        {
            free(dst_dir);
            errno = ENOMEM;
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            free_command_context(context);
            return 10;
        }
    }
    
    // Retrieves directory's content
    struct dirent **namelist;
    int n = scandir(context->base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        free(dst_dir);
        free_extensions(ext, ext_counter);
        fprintf(stderr, "Error on scandir(): %s\n", strerror(errno));
        free_command_context(context);
        return 6;
    }

    size_t moved_files = 0, created_directories = 0;

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") !=0)
        {
            move_element(context->base_dir, dst_dir, opts, namelist[i], ext,
                         ext_counter, &moved_files, &created_directories);
        }

        free(namelist[i]);
    }

    free(dst_dir);
    free(namelist);
    free_extensions(ext, ext_counter);

    // Output Message
    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] Files that would be moved: %zu\n"
               "[DRY-RUN] Directories that would be created: %zu\n",
               moved_files, created_directories);
    }
    else
    {
        printf("Files moved: %zu\n"
               "Created directories: %zu\n", moved_files, created_directories);
    }

    free_command_context(context);
    return 0;
}

// Creates destination directory
static char *get_valid_destination(const char *path)
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

// Parses through CLI arguments for 'move' functionality
int parse_move_opts(int argc, char **argv, int opt_start, void *opts_out)
{
    MoveOptions *opts = (MoveOptions*)opts_out;

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

    static struct option long_opts[] =
    {
        {"force", no_argument, 0, 'f'},
        {"skip", no_argument, 0, 's'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "fs";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'f':
            {
                opts->force = true;
                opts->skip = false;
                break;
            }
            case 's':
            {
                opts->force = false;
                opts->skip = true;
                break;
            }
            case '?':
            {
                break;
            }
        }
    }
    
    return 0;
}

// Move files from one directory to another
static void move_element(char *current_path, char *dst_dir, MoveOptions *opts,
                         struct dirent *namelist, Extension *ext, size_t ext_counter,
                         size_t *moved_files, size_t *created_directories)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    // Creates path to origin
    char new_path[PATH_MAX];
    if (check_path_name_size(new_path, sizeof(new_path), current_path, namelist->d_name) == -1)
    {
        return;
    }
    
    struct stat st;
    if (stat(new_path, &st) != 0)
    {
        return;
    }

    // Creates path to new directory (destination)
    char new_dst_dir[PATH_MAX];
    if (check_path_name_size(new_dst_dir, sizeof(new_dst_dir), dst_dir, namelist->d_name) == -1)
    {
        return;
    }

    // Recursively calls function on subdirectories
    if (S_ISDIR(st.st_mode))
    {
        if (is_directory_type(opts->filter.type))
        {
            bool can_move_whole = true;

            // Checks for name equality (contains)
            if (opts->filter.contains != NULL && opts->filter.contains[0] != '\0')
            {
                if (!match_name(opts->filter.contains, namelist->d_name))
                {
                    can_move_whole = false;
                }
            }

            if ((opts->filter.max_size || opts->filter.min_size) && can_move_whole)
            {
                off_t dir_size = 0;
                if (!match_directory_size(new_path, opts->filter.max_size, opts->filter.min_size, &dir_size))
                {
                    can_move_whole = false;
                }
            }

            if (can_move_whole)
            {                
                // Gets user's confirmation before creating new directory
                if (opts->action.interactive)
                {
                    char *prompt = NULL;
                    if (asprintf(&prompt, "Moves directory %s", new_dst_dir) == -1)
                    {
                        fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                        free(prompt);
                        return;
                    }            
                    if (!get_answer(prompt))
                    {
                        free(prompt);
                        return;
                    }

                    free(prompt);
                }
                if (opts->action.dry_run)
                {
                    printf("[DRY-RUN] would move directory %s\n", new_dst_dir);
                }
                else
                {
                    // Creates new subdirectory on destination
                    if (rename(new_path, new_dst_dir) == 0)
                    {
                        if (opts->action.verbose)
                        {
                            printf("Directory %s moved at %s\n", namelist->d_name, dst_dir);
                        }
                    }
                }

                return;
            }
        }

        if (opts->base.recursive)
        {
            // Gets user's confirmation before creating new directory
            if (opts->action.interactive)
            {
                char *prompt = NULL;
                if (asprintf(&prompt, "Moves directory %s", new_dst_dir) == -1)
                {
                    fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
                    free(prompt);
                    return;
                }            
                if (!get_answer(prompt))
                {
                    free(prompt);
                    return;
                }

                free(prompt);
            }
            if (opts->action.dry_run)
            {
                printf("[DRY-RUN] would move directory %s\n", new_dst_dir);
            }
            else
            {
                if (mkdir(new_dst_dir, 0755) != 0 && errno != EEXIST)
                {
                    return;
                }
            }

            (*created_directories)++;

            if (opts->action.verbose)
            {
                printf("Directory moved: %s\n", new_dst_dir);
            }

            // Retrieves all content from base directory
            struct dirent **entry;
            int n = scandir(new_path, &entry, NULL, alphasort);
            if (n == -1)
            {
                return;
            }

            for (int i = 0; i < n; i++)
            {
                move_element(new_path, new_dst_dir, opts, entry[i], ext,
                            ext_counter, moved_files, created_directories);
                free(entry[i]);
            }

            free(entry);
        }

        return;
    }

    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
    {
        return;
    }

    // Checks for element's type
    if (opts->filter.type && !match_type(opts->filter.type, st.st_mode))
    {
        return;
    }

    // Checks for name equality (contains)
    if (opts->filter.contains != NULL && opts->filter.contains[0] != '\0')
    {
        if (!match_name(opts->filter.contains, namelist->d_name))
        {
            return;
        }
    }
    // Checks for already existed file (force, skip)
    bool existed = access(new_dst_dir, F_OK) == 0;
    
    char *final_dst = NULL;
    if (existed)
    {
        final_dst = match_existed_file(dst_dir, new_dst_dir, namelist->d_name, opts->skip, opts->force);
        if (!final_dst)
        {
            return;
        }
    }
    else
    {
        final_dst = strdup(new_dst_dir);
    }

    // Checks for extension
    if (ext != NULL && !match_extension(ext, ext_counter, namelist->d_name))
    {
        free(final_dst);
        return;
    }

    // Checks for size
    if ((opts->filter.max_size || opts->filter.min_size) &&
        !match_size(opts->filter.max_size, opts->filter.min_size, st.st_size))
    {
        free(final_dst);
        return;
    }

    // Gets user's confirmation before moving file
    if (opts->action.interactive)
    {
        char *prompt = NULL;
        if (asprintf(&prompt, "Move file '%s' from %s to %s?", namelist->d_name, current_path, final_dst) == -1)
        {
            fprintf(stderr, "Error on asprintf(): %s\n", strerror(errno));
            free(prompt);
            return;
        }
        if (!get_answer(prompt))
        {
            free(prompt);
            return;
        }

        free(prompt);
    }

    if (opts->action.dry_run)
    {
        printf("[DRY-RUN] would move file '%s' from '%s' to '%s'\n", namelist->d_name, current_path, dst_dir);
    }
    else
    {
        // Moves file from source to destination
        if (rename(new_path, final_dst) == 0)
        {
            // Prints action in terminal
            if (opts->action.verbose)
            {
                printf("File '%s' moved from '%s' to '%s'\n", namelist->d_name, current_path, dst_dir);
            }
        }
    }

    (*moved_files)++;

    free(final_dst);
}

// Defines behaviour when file already exists on destination
static char *match_existed_file(char *dst_dir, char *new_dst_dir, char *name, bool skip, bool force)
{
    if (skip)
    {
        return NULL;
    }

    if (force)
    {
        remove(new_dst_dir);
        return strdup(new_dst_dir);
    }

    // Default behavior: incremental rename
    const char *dot = strrchr(name, '.');
    const char *valid_dot = (dot != NULL) ? dot : "";

    char base[PATH_MAX];
    if (dot)
    {
        ptrdiff_t len = dot - name;  // strlen(name) - strlen(valid_dot);
        strncpy(base, name, (size_t)len);
        base[len] = '\0';
    }
    else
    {
        strcpy(base, name);
    }

    char new_name[PATH_MAX];
    char final_path[PATH_MAX];
    size_t counter = 1;

    while (1)
    {
        snprintf(new_name, sizeof(new_name), "%s_%zu%s", base, counter, valid_dot);

        if (check_path_name_size(final_path, sizeof(final_path), dst_dir, new_name) == -1)
        {
            return NULL;
        }

        if (access(final_path, F_OK) != 0)
        {
            return strdup(final_path);
        }

        counter++;
    }
}
