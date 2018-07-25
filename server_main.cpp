#include "Server.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <csignal>

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

    // Create daemon
    if (to_start) {
        std::ifstream pid_file("/tmp/log.txt", std::ifstream::in); // File with server pid
        if (pid_file.good()) {
            std::cout << "Server already running" << std::endl;
            pid_file.close();
            exit(EXIT_FAILURE);
        } else {
            std::cout << "Creating server..." << std::endl;
        }

        pid_t process_id = 0;
        pid_t sid = 0;

        // Create child process
        process_id = fork();

        // Indication of fork() failure
        if (process_id < 0) {
            perror("fork failed!");
            exit(EXIT_FAILURE);
        }

        // Kill parent process
        if (process_id > 0) {
            std::cout << "Server process id " << process_id << std::endl;
            std::ofstream out_pid_file("/tmp/log.txt");

            if (out_pid_file.is_open()) {
                out_pid_file << process_id << std::endl;
                out_pid_file.close();
            } else {
                perror("Failed to write pid to file");
                exit(EXIT_FAILURE);
            }
            // return success in exit status
            exit(EXIT_SUCCESS);
        }

        // Unmask the file mode
        umask(0);

        // Set new session
        sid = setsid();
        if (sid < 0) {
            // Return failure
            exit(EXIT_FAILURE);
        }

        // Change current directory
        chdir("/");

        // Close stdin. stdout and stderr
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        // Create server
        Server server;
        server.HandleConnections();
    }

    // Read server process id from file, kill process and remove file
    if (to_kill) {
        std::cout << "KILL" << std::endl;
        std::ifstream pid_file("/tmp/log.txt");
        int pid;
        pid_file >> pid;

        int code_kill = kill(pid, SIGKILL);
        if (code_kill != 0)
            perror("Proc kill error");
        int code_remove = remove("/tmp/log.txt");

        if (code_remove != 0)
            perror("delete file error");
    }

    return 0;
}

