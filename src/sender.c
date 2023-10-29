#include "rtp.h"
#include "util.h"

int main(int argc, char **argv) {
    if (argc != 6) {
        LOG_FATAL("Usage: ./sender [receiver ip] [receiver port] [file path] "
                  "[window size] [mode]\n");
    }

    // your code here

    LOG_DEBUG("Sender: exiting...\n");
    return 0;
}
