#ifndef GENERAL_COMM_HPP
#define GENERAL_COMM_HPP

#include "Packet.hpp"
#include "Typedefs.hpp"

#include <map>

class GeneralComm {
    public:
        int read_pkt(int socket, packet* pkt);
        int write_pkt(int socket, packet pkt);

    protected:
        void init();

        std::map<int, str_pair> servers_info;
        void read_servers_info();

        void error(std::string error_message);
};

#endif