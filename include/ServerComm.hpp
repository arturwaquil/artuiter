#ifndef SERVER_COMM_HPP
#define SERVER_COMM_HPP

#include "Packet.hpp"

class ServerComm {
    public:
        ServerComm();
        ~ServerComm();
        int get_sockfd();
        int read_pkt(int socket, packet* pkt);
        int write_pkt(int socket, packet pkt);
        std::pair<int,int> _accept();

        void set_quit();

    private:
        int _create();
        int _bind();
        int _listen();

        void error(std::string error_message);

        int socket_file_descriptor;
        int port;

        bool quit;
};

typedef struct _client_thread_params
{
    std::string username;
    ServerComm* comm_manager;
    std::pair<int,int> sockets;

} client_thread_params;

client_thread_params create_client_thread_params(std::string username, std::pair<int,int> sockets);

#endif