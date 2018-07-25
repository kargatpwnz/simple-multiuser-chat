#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Client.h"

Client::Client()
{
}

Client::Client(const std::string& ip_, int port_num_)
        :ip_(ip_), port_num_(port_num_)
{
    std::cout << ip_ << " " << port_num_ << std::endl;
}

void Client::Connect()
{
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr_, 0, sizeof server_addr_);
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_num_);
    server_addr_.sin_addr.s_addr = inet_addr(ip_.data());

    if (connect(client_sock, (struct sockaddr *)&server_addr_, sizeof server_addr_) < 0)
        perror("Connect error");
    std::cout << "connected!" << std::endl;
    int tid_recv, tid_send;

    tid_send = pthread_create(&send_thread, nullptr, Client::SendHandler, this);
    if (tid_send)
        perror("thread send error");

    tid_recv = pthread_create(&recv_thread, nullptr, Client::RecvHandler, this);
    if (tid_recv)
        perror("thread recv error");

    std::cout << "Threads are created!" << std::endl;
    std::cout << "\nWelcome to chat!" << std::endl;
    std::cout << "type :exit to leave" << std::endl;
    pthread_join(send_thread, nullptr);
    pthread_join(recv_thread, nullptr);

}
void *Client::RecvHandler(void *arg)
{
    char buffer[1024];
    auto *client = (Client *) arg;
    ssize_t n;
    while (1) {
        memset(buffer, 0, sizeof buffer);
        n = recv(client->client_sock, buffer, sizeof buffer, 0);
        if (n <= 0) {
            std::cout << "Disconnect" << std::endl;
            pthread_exit(nullptr);
        }
//        if (strcmp(buffer, "SERVER_REJECTED_CONNECTION") == 0) {
//            std::cout << "Max connection reached!" << std::endl;
//            exit(EXIT_FAILURE);
//            break;
//        }
        std::cout << buffer << std::endl;
    }
}

void *Client::SendHandler(void *arg)
{
    auto *client = (Client *) arg;
    char *buffer = nullptr;
    size_t len = 0;
    while (1) {
        getline(&buffer, &len, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        if (strcmp(buffer, ":exit") == 0) {
            shutdown(client->client_sock, SHUT_RDWR);
            close(client->client_sock);
            break;
        }
        send(client->client_sock, buffer, strlen(buffer), 0);
    }
    pthread_exit(nullptr);
}
