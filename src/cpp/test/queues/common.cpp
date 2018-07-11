
// local includes
#include "common.h"

// std includes
#include <sys/stat.h>
#include <signal.h>

volatile int run_flag = 1;
static void stop_all(int x) {
    run_flag = 0;
}

void setupSignals() {
    signal(SIGINT, stop_all);
    signal(SIGTERM, stop_all);
    siginterrupt(SIGINT, 1);
    siginterrupt(SIGTERM, 1);
}
