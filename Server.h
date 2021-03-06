//
// Created by na1l on 24.07.18.
//

#ifndef SIBERS_SERVER_H
#define SIBERS_SERVER_H

#include "Client.h"

#include <fstream>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <vector>

class Server {
public:
    Server();
    virtual ~Server();
    void CreateAndStartDaemon();
    void HandleConnections();
    void CreateSocketAndListen();
    void KillDaemon();
private:
    static std::vector<Client*> clients_;

    int server_sock_;
    struct sockaddr_in server_addr_, client_addr_; // socket structures
    int port_num_ = 1500;

    static pthread_mutex_t mutex_; // mutex for critical sections
    pthread_t threads_[10]; // threads for connection handle
    static int idx_; // index for new connections

    bool is_socket_created_ = false;
    std::ofstream log_file_;
    static int FindClientIndex(Client* client);
    static void* HandleClient(void* arg);
    static void BcastMessage(const char* msg, Client* client);
    static void SendOnline(unsigned online);
    static void BcastMessage(const char* msg);

};

#endif //SIBERS_SERVER_H
