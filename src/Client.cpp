#include "../include/ClientComm.hpp"
#include "../include/ClientUI.hpp"
#include "../include/Signal.hpp"

#include <atomic>
#include <iostream>

std::atomic<bool> quit(false);    // signal flag

void sigIntHandler(int signum)
{
    quit.store(true);
}

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

    // Set sigIntHandler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sigIntHandler);

    std::cout << "Client initialized." << std::endl;

    packet pkt;

    while(!quit.load())
    {
        // Read message (command) from user. If empty, ignore. If EOF, exit.
        std::string message = ui.read();
        if (std::cin.eof()) break;
        if (message.empty()) continue;

        if (quit.load()) break;

        // Send command to server
        pkt = create_packet(command, 2, 1234, message);
        comm_manager.write_pkt(pkt);

        if (quit.load()) break;

        // Receive server's reply to the command
        comm_manager.read_pkt(&pkt);
        std::cout << pkt.payload << std::endl;
    }

    // Notify server that client is down
    comm_manager.write_pkt(create_packet(client_halt, 0, 1234, ""));

    std::cout << std::endl << "Exiting..." << std::endl;

    return 0;
}