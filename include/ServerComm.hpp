#ifndef SERVER_COMM_HPP
#define SERVER_COMM_HPP

#include <cstdint>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "ClientUI.hpp"

typedef struct _packet
{
    uint16_t type;  // DATA or CMD. TODO: is it necessary?
    uint16_t seqn;
    uint16_t timestamp;
    char payload[256];

} packet;

packet create_packet(uint16_t type, uint16_t seqn, uint16_t timestamp, std::string payload);

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

#endif