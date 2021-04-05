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

#endif