#ifndef CLIENT_COMM_HPP
#define CLIENT_COMM_HPP

#include "UI.hpp"
#include "Packet.hpp"

#include <string>

class ClientComm {
    public:
        ClientComm(std::string _hostname, std::string _port, UI _ui);
        ~ClientComm();
        int get_sockfd();
        int read_pkt(packet* pkt);
        int write_pkt(packet pkt);

    private:
        int _create();
        int _connect();

        void error(std::string error_message);

        int socket_file_descriptor;

        std::string hostname;
        std::string port;
        UI ui;
};

#endif