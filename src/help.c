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
        "   list        lists contens of a directory\n"
        "               Usage: ./archivist list DIRECTORY [FLAGS]\n"
        "               Use './archivis list help' for detailed flags and examples\n"
        "\n"
        "For more information about a specific command, use:\n"
        "   ./archivist COMMAND help\n"
        "Example: ./archivist list help"
    );
}
