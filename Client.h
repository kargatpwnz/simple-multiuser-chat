//
// Created by na1l on 24.07.18.
//

#ifndef SIBERS_CLIENT_H
#define SIBERS_CLIENT_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string>
#include <stdio.h>
#include <unistd.h>


class Client {
public:
    Client();
    Client(const std::string& ip_, int port_num_);
    std::string name;

    int client_sock;
    int id;
    void Connect();
private:
    struct sockaddr_in server_addr_;
    int port_num_;
    std::string ip_;
    pthread_t send_thread, recv_thread;

    static void *RecvHandler(void *arg);
    static void *SendHandler(void *arg);
};

#endif //SIBERS_CLIENT_H
