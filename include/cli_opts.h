#ifndef CLI_OPTS_H
#define CLI_OPTS_H

#include <stdbool.h>

// Structure of all CLI flags shared among files
typedef struct
{
    char *extension;
    bool human_readable;
    bool ignore_case;
    bool recursive;
    char *sort;
} GeneralOptions;

#endif
