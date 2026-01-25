#include "conf.h"

#include "cmd/cmd.h"

int
main(int argc, char **argv) {
#ifndef NDEBUG
    log_set_level(DEBUG);
#endif

    return cmd_parse(argc, argv);
}
