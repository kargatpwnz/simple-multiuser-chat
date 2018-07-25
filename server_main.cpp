#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Server.h"

void print_usage()
{
    std::cout << "Usage:\n\t./server_main -s | --start | -k | --kill" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "\t-s --start - start server daemon" << std::endl;
    std::cout << "\t-k --kill  - kill server daemon" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    int opt_idx;

    // Command-line arguments
    const char* short_opts = "sk";
    const struct option long_opts[] = {
            {"start", no_argument, nullptr, 's'},
            {"kill", no_argument, nullptr, 'k'},
            {0, 0, 0, 0}
    };

    bool to_start = false;
    bool to_kill = false;
    int opt;

    // Parse arguments
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &opt_idx)) != -1) {
        switch (opt) {
        case 's':
            to_start = true;
            break;
        case 'k':
            to_kill = true;
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    Server server;

    // Create daemon
    if (to_start) {
        server.CreateSocketAndListen();
        server.CreateAndStartDaemon();
    }

    // Kill server daemon
    if (to_kill) {
        server.KillDaemon();
    }

    return 0;
}

