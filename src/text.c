#include "cmd/cmd.h"

#include <stdlib.h>

int
main(int argc, char **argv) {
    if (argc < 2) {
        cmd_help(argc, argv);
        return EXIT_FAILURE;
    }

    return cmd_parse(argc, argv);
}
