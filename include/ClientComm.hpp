#ifndef CLIENT_COMM_HPP
#define CLIENT_COMM_HPP

#include "ClientUI.hpp"
#include "Packet.hpp"

#include <map>
#include <string>

class ClientComm {
    public:
        ClientComm();
        ~ClientComm();
        void init();

        int get_cmd_sockfd();
        int get_ntf_sockfd();

        int read_pkt(int socket, packet* pkt);
        int write_pkt(int socket, packet pkt);

    private:
        void read_servers_info();
        int _create();
        int _connect_to_primary();
        int _connect(std::string hostname, std::string port);
        void _disconnect();

        void error(std::string error_message);

        int cmd_sockfd;
        int ntf_sockfd;

        std::map<int, str_pair> servers_info;
};

#endif