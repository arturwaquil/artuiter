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
#include "../include/ClientComm.hpp"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: " << argv[0] << " <hostname> <port>\n";
        exit(EXIT_SUCCESS);
    }

    std::cout << "Initializing client..." << std::endl;

    ClientUI ui = ClientUI();
    ClientComm comm_manager = ClientComm(argv[1], argv[2], ui);

    std::cout << "Client initialized." << std::endl;

    packet pkt;

    while(true)
    {
        std::string message = ui.read();
        pkt = create_packet(1, 2, 1234, message);
        comm_manager.write_pkt(pkt);

        comm_manager.read_pkt(&pkt);
        std::cout << pkt.payload << std::endl;
    }

    std::cout << "Exiting..." << std::endl;

    return 0;
}