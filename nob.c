#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define SRC_FOLDER "src/"
#define CFLAGS "-Wall", "-Wextra", "-lncurses"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    cmd_append(&cmd, "cc", "src/main.c", CFLAGS, "-o", "tired");

    if (!nob_cmd_run_sync_and_reset(&cmd))
        return 1;

    return 0;
}
