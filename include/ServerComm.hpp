#ifndef SERVER_COMM_HPP
#define SERVER_COMM_HPP

#include <cstdint>
#include <unistd.h>
#include <string>
#include <cstring>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "ClientUI.hpp"
#include "Packet.hpp"

class ServerComm {
    public:
        ServerComm();
        ~ServerComm();
        int get_sockfd();
        int read_pkt(int socket, packet* pkt);
        int write_pkt(int socket, packet pkt);
        int _accept();

    private:
        int _create();
        int _bind();
        int _listen();

        int socket_file_descriptor;

        int port;
};

typedef struct _client_thread_params
{
    ServerComm* comm_manager;
    int new_sockfd;
    pthread_mutex_t* comm_manager_lock;

} client_thread_params;

client_thread_params create_client_thread_params(ServerComm* comm_manager, int new_sockfd, pthread_mutex_t* comm_manager_lock);

#endif