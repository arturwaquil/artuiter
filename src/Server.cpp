#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/ServerComm.hpp"

int main()
{
    ServerComm comm_manager = ServerComm();

    int new_sockfd = comm_manager._accept();

    int n;
    packet pkt;
    while(strcmp(pkt.payload, "exit") != 0)
    {
        comm_manager.read_pkt(new_sockfd, &pkt);

        std::cout << "type: " << pkt.type << std::endl;
        std::cout << "seqn: " << pkt.seqn << std::endl;
        std::cout << "timestamp: " << pkt.timestamp << std::endl;
        std::cout << "payload: " << pkt.payload << std::endl;

        pkt = create_packet(1,-1,0, std::string("Message received!"));
        comm_manager.write_pkt(new_sockfd, pkt);
    }

    // Close sockets
    close(new_sockfd);

    return 0;
}