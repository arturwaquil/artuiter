#ifndef CLIENT_COMM_HPP
#define CLIENT_COMM_HPP

#include <cstdint>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "ClientUI.hpp"
#include "Packet.hpp"

class ClientComm {
    public:
        ClientComm(std::string _hostname, std::string _port, ClientUI _ui);
        ~ClientComm();
        int get_sockfd();
        int read_pkt(packet* pkt);
        int write_pkt(packet pkt);

    private:
        int _create();
        int _connect();

        int socket_file_descriptor;

        std::string hostname;
        std::string port;
        ClientUI ui;
};

#endif