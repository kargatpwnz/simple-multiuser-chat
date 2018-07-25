#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

#include "Server.h"

// Static variables
std::vector<Client *> Server::clients_;
pthread_mutex_t Server::mutex_;
int Server::idx_;

Server::Server()
{
    // Create socket, fill struct, then listen for connections
    if ((server_sock_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket create error");
        exit(EXIT_FAILURE);
    }

    int opt;
    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof opt) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr_, 0, sizeof server_addr_);
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(port_num_);

    if (bind(server_sock_, (struct sockaddr *) &server_addr_, sizeof server_addr_) < 0)
        perror("bind error");

    listen(server_sock_, 10);
}

void Server::HandleConnections()
{
    Client *client;
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
                perror("accept error");
            } else {
                pthread_create(&threads_[idx_], NULL, HandleClient, client);
                idx_ = clients_.size();
            }
        } /*else {
            int sock = accept(server_sock_, (struct sockaddr*) &client_addr_, &client_len);
            char err_msg[] = "SERVER_REJECTED_CONNECTION";
            send(sock, err_msg, sizeof err_msg, 0);
            close(sock);
        }
*/
    }
}

void *Server::HandleClient(void* arg)
{
    auto *client = (Client *) arg;
    char buffer[1024];
    ssize_t n;

    // Recv messages
    while (1) {
        memset(buffer, 0, sizeof buffer);
        n = recv(client->client_sock, buffer, sizeof buffer, 0);

        // Disconnect or another error indication
        if (n <= 0) {
            std::string msg = "\nClient " + client->name + " disconnected\n";
            std::cout << msg << std::endl;
            BcastMessage(msg.data(), client);
            idx_ = client->id;

            // Critical section, delete client after disconnect
            pthread_mutex_lock(&Server::mutex_);
            int index = FindClientIndex(client);
            Server::clients_.erase(clients_.begin() + index);
            shutdown(client->client_sock, SHUT_RDWR);
            close(client->client_sock);
            pthread_mutex_unlock(&Server::mutex_);

            SendOnline(clients_.size());
            break;
        }  else {
            // Send message to all clients
            std::string str = "Client " + client->name + ": ";
            str += buffer;
            BcastMessage(str.data(), client);
        }
    }
}

void Server::BcastMessage(const char* msg, Client *c)
{
    // Critical section, send message to all client except sender
    pthread_mutex_lock(&Server::mutex_);
    for (auto client : Server::clients_) {
        if (client->client_sock == c->client_sock  && c != nullptr)
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
