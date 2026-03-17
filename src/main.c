#include "archivist.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {        
        fprintf(stderr, "Usage: ./archivist COMMAND [arguments...]\n,"
                        "Check available commands with: ./archivist help\n");
        return 1;
    }

    return execute_command(argc, argv);
}