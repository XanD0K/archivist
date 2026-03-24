#include <stdio.h>

void handle_help(void)
{
    puts(
        "Usage: ./archivist COMMAND [arguments]\n"
        "\n"
        "All commands:\n"
        "   help        shows this help message\n"
        "               Usage: ./archivist help\n"
        "\n"
        "   list        lists contents of a directory\n"
        "               Usage: ./archivist list DIRECTORY [FLAGS]\n"
        "\n"
        "   search      searches for an element (file, slink, directory)\n"
        "               Usage: ./archivist search NAME [DIRECTORY] [FLAGS]\n"
        "\n"
        "For more information about a specific command, use:\n"
        "   ./archivist COMMAND help\n"
        "Example:\n"
        "   ./archivist list help\n"
        "   ./archivist search help"
    );
}
