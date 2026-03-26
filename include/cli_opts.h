#ifndef CLI_OPTS_H
#define CLI_OPTS_H

#include <stdbool.h>

typedef struct
{
    char *extension;
    bool human_readable;
    bool ignore_case;
    bool recursive;
} GeneralOptions;

#endif
