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
        // Read message (command) from user. If empty, ignore. If EOF, exit.
        std::string message = ui.read();
        if (std::cin.eof()) break;
        if (message.empty()) continue;

        // Send command to server
        pkt = create_packet(command, 2, 1234, message);
        comm_manager.write_pkt(pkt);

        // Receive server's reply to the command
        comm_manager.read_pkt(&pkt);
        std::cout << pkt.payload << std::endl;
    }

    std::cout << std::endl << "Exiting..." << std::endl;

    return 0;
}