#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64  // Forces off_t to be 64 bits

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "search_cmd.h"
#include "utils.h"

// Globals
static const off_t BUFFER = 1024;
static char *base_dir;

// Prototypes
static void print_search_help(void);
static SearchOptions parse_search_opts(int argc, char **argv, int opt_start, bool *size_err);
static off_t get_size(char *size);
static void search_element(char *base_dir, SearchOptions opts, struct dirent *namelist, const char *searched_name);
static bool match_name(char *current_name, const char *searched, bool contains, bool ignore_case);
static bool match_extension(char *current_name, char *ext);
static bool match_type(char *current_name, char *current_path, char *type);
static bool match_size(char *current_name, char *current_path, off_t max_size, off_t min_size);

// Searches for a specific file/directory
int handle_search(int argc, char **argv)
{
    // Checks for 'help' flag
    if (argc == 3 &&
        (strcasecmp(argv[2], "help") == 0 ||
         strcasecmp(argv[2], "-h") == 0 ||
         strcasecmp(argv[2], "--help") == 0))
    {
        print_search_help();
        return 0;
    }

    const char *dir_path = NULL;
    int opt_start = 0;
    char *searched_name = NULL;

    if ((argc > 3 && argv[3][0] == '-') || argc == 3)
    {
        opt_start = 3;
        searched_name = strdup(argv[2]);
        if (!searched_name)
        {
            fprintf(stderr, "Couldn't duplicate string '%s': %s\n", argv[2], strerror(errno));
            return 4;
        }
    }
    else if (argc > 3 && argv[3][0] != '-')
    {
        dir_path = argv[2];
        opt_start = 4;
        searched_name = strdup(argv[3]);
        if (!searched_name)
        {
            fprintf(stderr, "Couldn't duplicate string '%s': %s\n", argv[3], strerror(errno));
            return 4;
        }
    }

    // Gets valid directory (default: .)
    base_dir = get_valid_directory(dir_path);
    if (!base_dir)
    {
        return 8;
    }

    bool size_err = false;
    // Parse CLI arguments
    SearchOptions opts = parse_search_opts(argc, argv, opt_start, &size_err);
    if (size_err)
    {
        errno = EIO;
        fprintf(stderr, "Invalid size: %s", strerror(errno));
        return 9;
    }

    struct dirent **namelist;
    int n = scandir(base_dir, &namelist, NULL, alphasort);

    if (n == -1)
    {
        perror("scandir");
        return 6;
    }

    for (int i = 0; i < n; i++)
    {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            search_element(base_dir, opts, namelist[i], searched_name);
            free(namelist[i]);
        }
    }

    free(namelist);
    free(base_dir);
    free(searched_name);
    return 0;
}

// Prints explanations to 'search' functionality
static void print_search_help(void)
{
    puts(
        "Usage: ./archivist search [DIRECTORY] NAME [FLAGS]\n"
        "\n"
        "DIRECTORY defaults to current directory (.)\n"
        "If NAME is not important, use * symbol"
        "\n"
        "Flags:\n"
        "   -c | --contains\n"
        "       includes all files/directories that look alike given name\n"
        "       default: off\n"
        "   -e | --extension\n"
        "       search for specific extension\n"
        "       e.g. txt | .txt"
        "   -i | --ignore-case\n"
        "       distinguish case\n"
        "       default: on\n"
        "   -t | --type\n"
        "       includes only specific type (file|dir|slink)\n"
        "   -R | --recursive\n"
        "       also search in subdirectories\n"
        "       default: off\n"
        "   --min-size\n"
        "   --max-size\n"
        "       only consider files smaller/larger than specified value\n"
        "       notation: B | K | KB | M | MB | G | GB | T | TB\n"
        "       If notation not specified, B will be considered\n"
        "       e.g. 20 | 50K | 30GB | 200T\n"
        "\n"
        "Examples:\n"        
        "./archivist search filename.txt\n"
        "./archivist search /folder filename.txt \n"
        "./archivist search /folder filename -e .txt \n"
        "./archivist search /folder * -e txt \n"
        "./archivist search /folder filen -c --ignore-case -t file\n"
        "./archivist search /folder filen -R --min-size 50K --max-size 50G\n"
        "\n"
        "All comands: ./archivist help"
    );
}

// Parse through CLI arguments for 'search' functionality
static SearchOptions parse_search_opts(int argc, char **argv, int opt_start, bool *size_err)
{
    SearchOptions opts = {0};

    // Set default values
    opts.base.ignore_case = true;

    static struct option long_opts[] =
    {
        {"recursive", no_argument, 0, 'R'},
        {"ignore-case", no_argument, 0, 'i'},
        {"contains", no_argument, 0, 'c'},
        {"min-size", required_argument, 0, 0},
        {"max-size", required_argument, 0, 0},
        {"type", required_argument, 0, 't'},
        {"extension", required_argument, 0, 'e'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    int long_index = 0;
    char *short_opts = "Rict:e:";

    optind = opt_start;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'R':
            {
                opts.base.recursive = true;
                break;
            }
            case 'i':
            {
                opts.base.ignore_case = false;
                break;
            }
            case 'c':
            {
                opts.contains = true;
                break;
            }
            case 't':
            {
                opts.type = optarg;
                break;
            }
            case 'e':
            {
                opts.extension = optarg;
                break;
            }
            case 0:
            {
                if (strcmp(long_opts[long_index].name, "min-size") == 0)
                {
                    opts.min_size = get_size(optarg);
                }
                else if (strcmp(long_opts[long_index].name, "max-size") == 0)
                {
                    opts.max_size = get_size(optarg);
                }
                if (opts.max_size == -1 || opts.min_size == -1)
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

static off_t get_size(char *size)
{
    char *ptr;
    long num = strtol(size, &ptr, 10);
    errno = 0;
    if (errno == EINVAL || errno == ERANGE)
    {
        perror("strtol");
        return -1;
    }
    if (ptr == size)
    {
        fprintf(stderr, "No digitis were found\n");
        return -1;
    }

    num = (off_t)num;

    if(ptr == NULL)
    {
        return num;
    }
    else if (strcasecmp(ptr, "K") == 0 || strcasecmp(ptr, "KB") == 0)
    {
        return num * BUFFER;
    }
    else if (strcasecmp(ptr, "M") == 0 || strcasecmp(ptr, "MB") == 0)
    {
        return num * BUFFER * BUFFER;
    }
    else if (strcasecmp(ptr, "G") == 0 || strcasecmp(ptr, "GB") == 0)
    {
        return num * BUFFER * BUFFER * BUFFER;
    }
    else if (strcasecmp(ptr, "T") == 0 || strcasecmp(ptr, "TB") == 0)
    {
        return num * BUFFER * BUFFER * BUFFER * BUFFER;
    }

    return -1;
}

// Searches for an element
static void search_element(char *current_path, SearchOptions opts, struct dirent *namelist, const char *searched)
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

    // Checks recursively for searched element
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
            search_element(new_path, opts, entry[i], searched);
            free(entry[i]);
        }

        free(entry);
    }

    // Checks for name equality
    if (strcmp(namelist->d_name, "*") != 0 && !match_name(namelist->d_name, searched, opts.contains, opts.base.ignore_case))
    {
        return;
    }

    // Checks for extension
    if (opts.extension && !match_extension(namelist->d_name, opts.extension))
    {
        return;
    }

    // Checks for element's type
    if (opts.type && !match_type(namelist->d_name, current_path, opts.type))
    {
        return;
    }

    // Checks for size
    if ((opts.max_size || opts.min_size) && !match_size(namelist->d_name, current_path, opts.max_size, opts.min_size))
    {
        return;
    }

    // Prints found element
    const char *sufix = new_path + strlen(base_dir);
    if (*sufix == '/')
    {
        sufix++;
    }
    printf("%s\n", sufix);
    
    return;

}

// Checks if strings match
static bool match_name(char *current_name, const char *searched, bool contains, bool ignore_case)
{
    // Exact match
    if(!contains)
    {
        return (ignore_case) ? (strcasecmp(current_name, searched) == 0) : (strcmp(current_name, searched) == 0);
    }
    // Partial match (Contains = True)
    // Parses through all characters
}

// Checks if extension matches
static bool match_extension(char *current_name, char *ext)
{
    const char *dot = strrchr(current_name, '.');
    const char *ext_name = (dot != NULL) ? dot + 1 : "";
    const char *clean_ext = (ext[0] == '.') ? ext + 1 : ext;

    return (strcasecmp(ext_name, clean_ext) == 0);
}

// Checks if type matches
static bool match_type(char *current_name, char *current_path, char *type)
{
    struct stat st;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", current_path, current_name);
    if (stat(path, &st) != 0)
    {
        return false;
    }

    if (strcasecmp(type, "f") == 0 || strcasecmp(type, "file") == 0)
    {
        return S_ISREG(st.st_mode);
    }
    else if (strcasecmp(type, "d") == 0 || strcasecmp(type, "dir") == 0 || strcasecmp(type, "directory") == 0)
    {
        return S_ISDIR(st.st_mode);
    }
    else if (strcasecmp(type, "sl") == 0 || strcasecmp(type, "slink") == 0 || strcasecmp(type, "symbolic-link") == 0)
    {
        return S_ISLNK(st.st_mode);
    }
    return false;
}

// Checks if size is between min and max
static bool match_size(char *current_name, char *current_path, off_t max_size, off_t min_size)
{
    struct stat st;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", current_path, current_name);
    if (stat(path, &st) != 0)
    {
        return false;
    }

    if (max_size != 0 && min_size != 0)
    {
        return (st.st_size > min_size && st.st_size < max_size);
    }
    else if (max_size != 0)
    {
        return (st.st_size < max_size);
    }
    else
    {
        return (st.st_size > min_size);
    }
}
