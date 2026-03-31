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
static MoveOptions parse_move_opts(int argc, char*argv, int opt_start, bool *size_err);
static void move_element(char *current_dir, char *base_dir, char *dst_dir, MoveOptions opts, struct dirent *namelist);
static int check_directory_size(char *dst, size_t len, const char *prefix, const char *suffix);

// Moves files and subdirectories
int handle_move(int argc, char **argv)
{
    // Checks for 'help' flag
    if (check_help(argv, argv[2]))
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

    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, alphasort);
    if (n == -1)
    {
        free(base_dir);
        free(dst_dir);
        perror("scandir");
        return 9;
    }

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") !=0)
        {
            move_element(base_dir, base_dir, dst_dir, opts, namelist[i]);
            free(namelist[i]);
        }
    }

    free(namelist);

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
        "       Asks for confirmation before moving any file\n"
        "   -s | --skip\n"
        "       if file/subdirectory alredy exists, it won't be moved\n"
        "       default: off\n"
        "   -t | --type\n"
        "       moves only specific type (file | dir | slink)\n"
        "   -v | --verbose\n"
        "       states every moved file/subdirectory\n"
        "       default: off\n"
        "   -R | --recursive\n"
        "       also move in subdirectories\n"
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
    char *start_path = (cpy_path[0] == '/') ? "" : ".";
    char current_path[PATH_MAX];
    if (check_directory_size(current_path, sizeof(current_path), start_path, NULL) == -1)
    {
        free(cpy_path);
        fprintf(stderr, "Path too long: %s\n", strerror(errno));
        return NULL;
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
static MoveOptions parse_move_opts(int argc, char*argv, int opt_start, bool *size_err)
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

static void move_element(char *current_path, char *base_dir, char *dst_dir, MoveOptions opts, struct dirent *namelist)
{
    if (strcmp(namelist->d_name, ".") == 0 || strcmp(namelist->d_name, "..") == 0)
    {
        return;
    }

    struct stat st;
    char new_path[PATH_MAX];
    snprintf(new_path, sizeof(new_path), "%s/%s", current_path, namelist->d_name);
    if (stat(new_path, &st) != 0)
    {
        return;
    }

    // Recursively calls move function on subdirectories
    if (opts.base.recursive && S_ISDIR(st.st_mode))
    {
        struct dirent **entry;
        int n = scandir(new_path, &entry, NULL, alphasort);
        if (n == -1)
        {
            return;
        }

        for (int i = 0; i < n; i++)
        {
            move_element(new_path, base_dir, dst_dir, opts, entry[i]);
            free(entry[i]);
        }

        free(entry);
    }

    // Checks for flags
    // Checks for name equality (contains)
    if (!match_name(opts.contains, namelist->d_name))
    {
        return;
    }

    // Checks for already existed file (force, skip)
    if ((opts.force || opts.skip))

    // Checks for extension
    if (opts.base.extension && !match_extension(opts.base.extension, namelist->d_name))
    {
        return;
    }

    // Checks for element's type
    if (opts.filter.type && !match_type(opts.filter.type, st.st_mode))
    {
        return;
    }

    // Checks for size
    if ((opts.filter.max_size || opts.filter.min_size) && !match_size(opts.filter.max_size, opts.filter.min_size, st.st_size))
    {
        return;
    }

    opts.dry_run;
    opts.filter;
    opts.interactive;
    opts.verbose;

}

// Checks if name matches
static bool match_name(char *contains, const char *name)
{
    return (!contains || contains[0] == '\0')
        ? strcasecmp(contains, name) == 0
        : strcasestr(contains, name) == 0;
}