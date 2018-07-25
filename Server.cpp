#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "Server.h"

// Static variables
std::vector<Client*> Server::clients_;
pthread_mutex_t Server::mutex_;
int Server::idx_;

Server::Server()
{
    // Create file to save logs
    log_file_.open("/tmp/chat_server.log", std::ofstream::app);
}

Server::~Server()
{
}

void Server::HandleConnections()
{
    Client* client;
    idx_ = 0;
    socklen_t client_len = sizeof client_addr_;
    Server::mutex_ = PTHREAD_MUTEX_INITIALIZER;

    // Endless loop for connection handling
    while (1) {
        if (clients_.size() < 10) {
            client = new Client();
            client->client_sock = accept(server_sock_, (struct sockaddr*) &client_addr_, &client_len);
            client->id = idx_;
            client->name = std::to_string(idx_);

            pthread_mutex_lock(&Server::mutex_);
            Server::clients_.push_back(client);
            pthread_mutex_unlock(&Server::mutex_);
            SendOnline(clients_.size());
            if (client->client_sock < 0) {
                log_file_ << "accept error" << std::endl;
            } else {
                log_file_ << "Connected ip " << inet_ntoa(client_addr_.sin_addr) << ":" <<
                          ntohs(client_addr_.sin_port) << std::endl;
                pthread_create(&threads_[idx_], NULL, HandleClient, client);
                idx_ = clients_.size();
            }
        }
    }
}

void* Server::HandleClient(void* arg)
{
    auto* client = (Client*) arg;
    char buffer[1024];
    ssize_t n;

    // Recv messages
    while (1) {
        memset(buffer, 0, sizeof buffer);
        n = recv(client->client_sock, buffer, sizeof buffer, 0);

        // Disconnect or another error indication
        if (n <= 0) {
            std::string msg = "\nClient " + client->name + " disconnected\n";
            if (!clients_.empty())
                BcastMessage(msg.data(), client);

            // Critical section, delete client after disconnect, close socket
            pthread_mutex_lock(&Server::mutex_);
            int index = FindClientIndex(client);
            if (index == -1) {
                pthread_mutex_unlock(&Server::mutex_);
                pthread_exit(nullptr);
            }
            Server::clients_.erase(clients_.begin() + index);
            shutdown(client->client_sock, SHUT_RDWR);
            close(client->client_sock);

            // If all cients are gone start indexing from 0 again
            if (clients_.empty())
                idx_ = 0;
            else
                // New client idx would be last disconnected client
                idx_ = client->id;
            pthread_mutex_unlock(&Server::mutex_);

            if (!clients_.empty()) {
                SendOnline(clients_.size());
            }
        } else {
            // Send message to all clients
            std::string str = "Client " + client->name + ": ";
            str += buffer;
            BcastMessage(str.data(), client);
        }
    }
}

void Server::BcastMessage(const char* msg, Client* c)
{
    // Critical section, send message to all client except sender
    pthread_mutex_lock(&Server::mutex_);
    for (auto client : Server::clients_) {
        if (client->client_sock == c->client_sock && c != nullptr)
            continue;
        else
            send(client->client_sock, msg, strlen(msg), 0);
    }
    pthread_mutex_unlock(&Server::mutex_);
}

int Server::FindClientIndex(Client* client)
{
    for (unsigned i = 0; i < clients_.size(); ++i) {
        if (client->id == clients_[i]->id)
            return i;
    }
    return -1;
}

// Show online count to users
void Server::SendOnline(unsigned online)
{
    std::string message = "Online is " + std::to_string(online) + " people\n";
    BcastMessage(message.c_str());
}

// Send message to all users
void Server::BcastMessage(const char* msg)
{
    pthread_mutex_lock(&Server::mutex_);
    for (auto client : Server::clients_) {
        send(client->client_sock, msg, strlen(msg), 0);
    }
    pthread_mutex_unlock(&Server::mutex_);
}

void Server::CreateAndStartDaemon()
{
    if (!is_socket_created_) {
        std::cout << "Create socket first!" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::ifstream pid_file("/tmp/server_pid", std::ifstream::in); // File with server pid
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
        std::ofstream out_pid_file("/tmp/server_pid");

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
    log_file_ << "Daemon started" << std::endl;
    HandleConnections();
}

void Server::KillDaemon()
{
    std::cout << "Killing server daemon" << std::endl;
    std::ifstream pid_file("/tmp/server_pid");
    if (!pid_file.is_open()) {
        log_file_ << "Daemon process not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    int pid;
    pid_file >> pid;

    int code_kill = kill(pid, SIGKILL);
    if (code_kill != 0)
        log_file_ << "Proc kill error" << std::endl;

    int code_remove = remove("/tmp/server_pid");
    if (code_remove != 0)
        log_file_ << "delete file error" << std::endl;
    log_file_ << "Daemon killed" << std::endl;
}

void Server::CreateSocketAndListen()
{
    // Create socket, fill struct, then listen for connections
    if ((server_sock_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_file_ << "socket create error" << std::endl;
        exit(EXIT_FAILURE);
    } else {
        log_file_ << "socket created!" << std::endl;

    }

    int opt;
    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof opt) < 0) {
        log_file_ << "setsockopt error" << std::endl;
        exit(EXIT_FAILURE);
    }

    memset(&server_addr_, 0, sizeof server_addr_);
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(port_num_);

    if (bind(server_sock_, (struct sockaddr*) &server_addr_, sizeof server_addr_) < 0)
        log_file_ << "bind error" << std::endl;
    else
        log_file_ << "socket binded" << std::endl;

    log_file_ << "listening for new connections" << std::endl;
    listen(server_sock_, 10);
    is_socket_created_ = true;
}
