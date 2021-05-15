#ifndef CLIENT_COMM_HPP
#define CLIENT_COMM_HPP

#include "ClientUI.hpp"
#include "GeneralComm.hpp"
#include "Packet.hpp"

#include <string>

class ClientComm : public GeneralComm {
    public:
        ClientComm(){};
        ~ClientComm();
        void init();

        int get_cmd_sockfd();
        int get_ntf_sockfd();

    private:
        int _create();
        int _connect_to_primary();
        int _connect(std::string hostname, std::string port);
        void _disconnect();

        int cmd_sockfd;
        int ntf_sockfd;
};

#endif