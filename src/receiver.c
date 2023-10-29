#include "rtp.h"
#include "util.h"

int main(int argc, char **argv) {
    if (argc != 5) {
        LOG_FATAL("Usage: ./receiver [listen port] [file path] [window size] "
                  "[mode]\n");
    }

    // your code here

    LOG_DEBUG("Receiver: exiting...\n");
    return 0;
}
