#ifndef CLIENT_COMM_HPP
#define CLIENT_COMM_HPP

#include "UI.hpp"
#include "Packet.hpp"

#include <string>

class ClientComm {
    public:
        ClientComm();
        void init(std::string _hostname, std::string _port, UI _ui);
        ~ClientComm();
        int get_cmd_sockfd();
        int get_ntf_sockfd();
        int read_pkt(int socket, packet* pkt);
        int write_pkt(int socket, packet pkt);

    private:
        int _create();
        int _connect();

        void error(std::string error_message);

        int cmd_sockfd;
        int ntf_sockfd;

        std::string hostname;
        std::string port;
        UI ui;
};

#endif