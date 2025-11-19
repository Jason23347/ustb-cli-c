#include "conf.h"

#include "cmd/cmd.h"

#include <stdlib.h>

int
main(int argc, char **argv) {
#ifndef NDEBUG
    log_set_level(DEBUG);
#endif

    if (argc < 2) {
        cmd_help(argc, argv);
        return EXIT_FAILURE;
    }

    return cmd_parse(argc, argv);
}
