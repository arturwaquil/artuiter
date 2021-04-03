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
    if (argc < 4)
    {
        std::cout << "usage: " << argv[0] << "<username> <hostname> <port>\n";
        exit(EXIT_SUCCESS);
    }

    std::cout << "Initializing client..." << std::endl;

    ClientUI ui = ClientUI();
    ClientComm comm_manager = ClientComm(argv[2], argv[3], ui);

    // Set sigIntHandler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sigIntHandler);

    packet pkt;

    // Send login message, wait for positive reply
    pkt = create_packet(login, 0, 0, std::string(argv[1]));
    comm_manager.write_pkt(pkt);
    comm_manager.read_pkt(&pkt);
    if (pkt.type == reply_login)
    {
        if (pkt.payload == std::string("OK"))
        {
            std::cout << "User " << argv[1] << " logged in successfully." << std::endl;
        }
        else
        {
            std::cout << "[ERROR] Couldn't login, user " << argv[1] 
                        << " already has two connections to the server." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        std::cout << "[ERROR] Couldn't login." << std::endl;
        exit(EXIT_FAILURE);
    }

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