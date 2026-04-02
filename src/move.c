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

// Hedaers
#include "move.h"
#include "utils.h"
#include "utils_filter.h"

// Prototypes
static void print_move_help(void);
static char *get_valid_destination(const char *path);
static int check_directory_size(char *dst, size_t len, const char *prefix, const char *suffix);
static MoveOptions parse_move_opts(int argc, char **argv, int opt_start, bool *size_err);
static void move_element(char *current_path, char *base_dir, char *dst_dir, MoveOptions opts,
                         struct dirent *namelist, Extension *ext, size_t ext_counter,
                         size_t *moved_files, size_t *created_directories);
static bool match_name(char *contains, const char *name);
static bool match_extension(Extension *exts, size_t ext_counter, char *name);
static char *match_existed_file(char *dst_dir, char *new_dst_dir, char *name, bool skip, bool force);

// Moves files and subdirectories
int handle_move(int argc, char **argv)
{
    // Checks for 'help' flag
    if (check_help(argc, argv[2]))
    {
        print_move_help();
        return 0;
    }

    // Defines starting values
    const char *dir_path_src = NULL;
    const char *dir_path_dst = argv[2];
    int opt_start = 3;
    if (argc > 3 && argv[3][0] != '-')
    {
        opt_start = 4;
        dir_path_src = argv[2];
        dir_path_dst = argv[3];
    }

    // Validates base directory
    char *base_dir = get_valid_directory(dir_path_src);
    if (!base_dir)
    {
        return 4;
    }
    // Checks/Creates destination directory
    char *dst_dir = get_valid_destination(dir_path_dst);
    if (!dst_dir)
    {
        free(base_dir);
        return 4;
    }

    // Parses CLI arguments
    bool size_err = false;
    MoveOptions opts = parse_move_opts(argc, argv, opt_start, &size_err);
    if (size_err)
    {
        free(base_dir);
        free(dst_dir);
        errno = EIO;
        fprintf(stderr, "Invalid size: %s", strerror(errno));
        return 9;
    }

    // Retrieves directory's content
    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        free(base_dir);
        free(dst_dir);
        perror("scandir");
        return 9;
    }

    // Retrieves user's selected extensions (-e|--extension flag)
    Extension *ext = NULL;
    size_t ext_counter = 0;
    if (opts.base.extension && opts.base.extension[0] != '\0')
    {
        ext = get_all_extensions(opts.base.extension, &ext_counter);
        if (!ext)
        {
            free(base_dir);
            free(dst_dir);
            errno = ENOMEM;
            fprintf(stderr, "Error on memory allocation: %s\n", strerror(errno));
            return 10;
        }
    }

    size_t moved_files = 0, created_directories = 0;

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") !=0)
        {
            move_element(base_dir, base_dir, dst_dir, opts, namelist[i], ext,
                         ext_counter, &moved_files, &created_directories);
            free(namelist[i]);
        }
    }

    free(base_dir);
    free(dst_dir);
    free(namelist);

    if (opts.dry_run)
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

    return 0;
}


// Prints explanation of 'move' functionality
static void print_move_help(void)
{
    puts(
        "Usage: ./archivist move [DIRECTORY] DIRECTORY [FLAGS]\n"
        "\n"
        "First DIRECTORY is the ORIGIN. It defaults to current directory (.)\n"
        "Second DIRECTORY is the DESTINATION. It is a required argument\n"
        "If destination directory doesn't exist, it will be created\n"
        "If file already exists on destination, new file will be renamed (e.g. file.txt → file_2.txt)\n"
        "\n"
        "Flags:\n"
        "   -c | --contains\n"
        "       moves only files/subdirectories that contain a word/pattern\n"
        "   -d | --dry-run\n"
        "       simulates changes, showing the result\n"
        "       default: off\n"
        "   -e | --extension\n"
        "       move files of given extension\n"
        "       if more than 1 extension, separate them with a comma\n"
        "       e.g. txt | .txt\n"
        "   -f | --force\n"
        "       if file/subdirectory already exists, it will be overwritten\n"
        "       default: off\n"
        "   -i | --interactive\n"
        "       Asks for confirmation before moving file or creating directory\n"
        "   -s | --skip\n"
        "       if file/subdirectory alredy exists, it won't be moved\n"
        "       default: off\n"
        "   -t | --type\n"
        "       moves only specific type (file | dir | slink)\n"
        "   -v | --verbose\n"
        "       states every moved file/subdirectory\n"
        "       default: off\n"
        "   -R | --recursive\n"
        "       also moves all files from subdirectories\n"
        "       default: on\n"
        "   --min-size\n"
        "   --max-size\n"
        "       only consider files smaller/larger than specified value\n"
        "       notation: B | K | KB | M | MB | G | GB | T | TB\n"
        "       default: B (bytes)\n"
        "       e.g. 20 | 50K | 30GB | 200T\n"
        "\n"
        "Attention:\n"
        "'force' and 'skip' flags are excludent. If both are provided, the last one will prevail\n"
        "\n"
        "Examples:\n"
        "   ./archivist move folder2/\n"
        "   ./archivist move folder1/ folder2/\n"
        "   ./archivist move folder1/ folder2/ -c word\n"
        "   ./archivist move folder1/ folder2/ -d -e txt,pdf\n"
        "   ./archivist move folder1/ folder2/ --force -i --type file\n"
        "   ./archivist move folder1/ folder2/ -s -v -R\n"
        "   ./archivist move folder1/ folder2/ --min-size 50K --max-size 50G\n"        
        "\n"
        "All commands: ./archivist help"
    );
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
        if (check_directory_size(new_path, sizeof(new_path), current_path, token) == -1)
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
                perror("mkdir");
                return NULL;
            }
        }

        // Updates current path for recursiveness
        if (check_directory_size(current_path, sizeof(current_path), new_path, NULL) == -1)
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

// Checks if directory will overflow maximum size
static int check_directory_size(char *dst, size_t len, const char *prefix, const char *suffix)
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

// Parses through CLI arguments for 'move' functionality
static MoveOptions parse_move_opts(int argc, char **argv, int opt_start, bool *size_err)
{
    MoveOptions opts = {0};
    opts.base.recursive = true;

    static struct  option long_opts[] =
    {
        {"contains", no_argument, 0, 'c'},
        {"dry-run", no_argument, 0, 'd'},
        {"extension", required_argument, 0, 'e'},
        {"force", no_argument, 0, 'f'},
        {"interactive", no_argument, 0, 'i'},
        {"skip", no_argument, 0, 's'},
        {"type", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'v'},
        {"recursive", no_argument, 0, 'R'},        
        {"max-size", required_argument, 0, 0},
        {"min-size", required_argument, 0, 0},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "cde:fist:vR";

    // Defines starting index to search for arguments
    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'c':
            {
                opts.contains = optarg;
                break;
            }
            case 'd':
            {
                opts.dry_run = true;
                break;
            }
            case 'e':
            {
                opts.base.extension = optarg;
                break;
            }
            case 'f':
            {
                opts.force = true;
                opts.skip = false;
                break;
            }
            case 'i':
            {
                opts.interactive = true;
                break;
            }
            case 's':
            {
                opts.skip = true;
                opts.force = false;
                break;
            }
            case 't':
            {
                opts.filter.type = optarg;
                break;
            }
            case 'v':
            {
                opts.verbose = true;
                break;
            }
            case 'R':
            {
                opts.base.recursive = false;
                break;
            }
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "max-size") == 0)
                {
                    opts.filter.max_size = get_size(optarg);
                }
                else if (strcmp(long_opts[long_index].name, "min-size") == 0)
                {
                    opts.filter.min_size = get_size(optarg);
                }
                if (opts.filter.max_size == -1 || opts.filter.min_size == -1)
                {
                    *size_err = true;
                }
                break;
            }
            case '?':
            {
                break;
            }
        }
    }

    return opts;
}

static void move_element(char *current_path, char *base_dir, char *dst_dir, MoveOptions opts,
                         struct dirent *namelist, Extension *ext, size_t ext_counter,
                         size_t *moved_files, size_t *created_directories)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    // Checks for name equality (contains)
    if ((opts.contains != NULL && opts.contains[0] != '\0') &&
        !match_name(opts.contains, namelist->d_name))
    {
        return;
    }

    struct stat st;
    // Creates path to origin
    char new_path[PATH_MAX];
    if (check_directory_size(new_path, sizeof(new_path), current_path, namelist->d_name) == -1)
    {
        return;
    }
    if (stat(new_path, &st) != 0)
    {
        return;
    }

    // Creates path to new directory (destination)
    char new_dst_dir[PATH_MAX];
    if (check_directory_size(new_dst_dir, sizeof(new_dst_dir), dst_dir, namelist->d_name) == -1)
    {
        return;
    }

    // Recursively calls function on subdirectories
    if (opts.base.recursive && S_ISDIR(st.st_mode))
    {
        // Gets user's confirmation before creating new directory
        if (opts.interactive)
        {
            char prompt[PATH_MAX];
            snprintf(prompt, sizeof(prompt), "Creates directory %s", new_dst_dir);
            if (!get_answer(prompt))
            {
                return;
            }
        }

        if (opts.dry_run)
        {
            printf("[DRY-RUN] would create directory %s\n", new_dst_dir);
        }
        else
        {
            // Creates new subdirectory on destination
            if (mkdir(new_dst_dir, 0755) != 0 && errno != EEXIST)
            {
                return;
            }
        }

        (*created_directories)++;

        if (opts.verbose)
        {
            printf("Directory %s created at %s\n", namelist->d_name, dst_dir);
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
            move_element(new_path, base_dir, new_dst_dir, opts, entry[i], ext,
                         ext_counter, moved_files, created_directories);
            free(entry[i]);
        }

        free(entry);
        return;
    }

    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
    {
        return;
    }

    // Checks for element's type
    if (opts.filter.type && !match_type(opts.filter.type, st.st_mode))
    {
        return;
    }

    // Checks for already existed file (force, skip)
    bool existed = access(new_dst_dir, F_OK) == 0;
    
    char *final_dst = NULL;
    if (existed)
    {
        final_dst = match_existed_file(dst_dir, new_dst_dir, namelist->d_name, opts.skip, opts.force);
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
    if ((opts.filter.max_size || opts.filter.min_size) &&
        !match_size(opts.filter.max_size, opts.filter.min_size, st.st_size))
    {
        free(final_dst);
        return;
    }

    // Gets user's confirmation before moving file
    if (opts.interactive)
    {
        char prompt[PATH_MAX];
        snprintf(prompt, sizeof(prompt), "Move file '%s' from %s to %s?", namelist->d_name, current_path, final_dst);
        if (!get_answer(prompt))
        {
            return;
        }
    }

    if (opts.dry_run)
    {
        printf("[DRY-RUN] would move file '%s' from '%s' to '%s'\n", namelist->d_name, current_path, dst_dir);
    }
    else
    {
        // Moves file from source to destination
        rename(new_path, final_dst);
    }

    (*moved_files)++;

    // Prints action in terminal
    if (opts.verbose)
    {
        printf("File '%s' moved from '%s' to '%s'\n", namelist->d_name, current_path, dst_dir);
    }

    free(final_dst);
}

// Checks if name matches
static bool match_name(char *contains, const char *name)
{
    return (strcasestr(name, contains) != NULL);
}

// Checks if extension matches
static bool match_extension(Extension *exts, size_t ext_counter, char *name)
{
    const char *ext_name = get_clean_extension(name);
    if (!ext_name || ext_name[0] == '\0')
    {
        return false;
    }

    for (size_t i = 0; i < ext_counter; i++)
    {
        const char *clean_ext = exts[i].extension;
        
        if (clean_ext[0] == '.')
        {
            clean_ext++;
        }
        if (strcasecmp(ext_name, clean_ext) == 0)
        {
            return true;
        }
    }

    return false;
}

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
    char *dot = strrchr(name, '.');
    char *valid_dot = (dot != NULL) ? dot : "";

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
    size_t counter = 1;

    while (1)
    {
        snprintf(new_name, sizeof(new_name), "%s_%zu%s", base, counter, valid_dot);

        char final_path[PATH_MAX];
        if (check_directory_size(final_path, sizeof(final_path), dst_dir, new_name) == -1)
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
