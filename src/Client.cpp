#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../include/ClientUI.hpp"

typedef struct _packet
{
    uint16_t type;  // DATA or CMD. TODO: is it necessary?
    uint16_t seqn;
    uint16_t timestamp;
    char payload[256];

} packet;

int main(int argc, char *argv[])
{
    int n;
    sockaddr_in server_address;
    hostent *server;
    packet pkt;

    if (argc < 3)
    {
        std::cout << "usage: " << argv[0] << " <hostname> <port>\n";
        exit(EXIT_SUCCESS);
    }

    ClientUI ui = ClientUI();
    
    server = gethostbyname(argv[1]);
    if (server == NULL) exit(EXIT_FAILURE);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) exit(EXIT_FAILURE);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[2]));
    server_address.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(server_address.sin_zero), 8);     
    
    int connected = connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address));
    if (connected < 0) exit(EXIT_FAILURE);

    while(true)
    {
        strcpy(pkt.payload, ui.read().c_str());

        pkt.type = 1;
        pkt.seqn = 2;
        pkt.timestamp = 1234;

        n = write(sockfd, &pkt, sizeof(pkt));
        if (n < 0) exit(EXIT_FAILURE);

        char buffer[256];
        bzero(buffer, 256);
        n = read(sockfd, &buffer, 256);
        if (n < 0) exit(EXIT_FAILURE);

        std::cout << buffer << std::endl;
    }

    close(sockfd);

    return 0;
}