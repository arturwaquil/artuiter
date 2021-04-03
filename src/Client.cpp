#include "../include/ClientComm.hpp"
#include "../include/UI.hpp"
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
        std::cout << "usage: " << argv[0] << " @<username> <hostname> <port>" << std::endl;
        exit(EXIT_SUCCESS);
    }

    std::string username = std::string(argv[1]);

    // Assert username format (@<username>) and size (4–20) as per the specification
    if (username[0] != '@')
    {
        std::cout << "[ERROR] Must insert an at sign (@) before the username." << std::endl;
        exit(EXIT_FAILURE);
    }
    if (username.length() < 5 || username.length() > 21)
    {
        std::cout << "[ERROR] Invalid username. Username must be between 4 and 20 characters long." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Initializing client..." << std::endl;

    UI ui = UI();
    ClientComm comm_manager = ClientComm(argv[2], argv[3], ui);

    // Set sigIntHandler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sigIntHandler);

    packet pkt;

    // Send login message, wait for positive reply
    pkt = create_packet(login, 0, 0, username);
    comm_manager.write_pkt(pkt);
    comm_manager.read_pkt(&pkt);
    if (pkt.type == reply_login)
    {
        if (pkt.payload == std::string("OK"))
        {
            ui.write("User " + username + " logged in successfully.");
        }
        else
        {
            ui.write("[ERROR] Couldn't login, user " + username + " already has two connections to the server.");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        ui.write("[ERROR] Couldn't login.");
        exit(EXIT_FAILURE);
    }

    ui.write("Commands: FOLLOW @<username> | SEND <message> | EXIT");

    while(!quit.load())
    {
        // Read message (command) from user. If empty, ignore. If EOF, exit.
        std::string message = ui.read();
        if (std::cin.eof()) break;
        if (message.empty()) continue;

        // Handle exit command similarly to SIGINT
        if (message == "EXIT" || message == "exit") break;

        if (quit.load()) break;

        // Send command to server
        pkt = create_packet(command, 0, 1234, message);
        comm_manager.write_pkt(pkt);

        if (quit.load()) break;

        // Receive server's reply to the command
        comm_manager.read_pkt(&pkt);
        ui.write(pkt.payload);
        if (pkt.payload == std::string("Unknown command.")) ui.write("Commands: FOLLOW @<username> | SEND <message> | EXIT");
    }

    // Notify server that client is down
    comm_manager.write_pkt(create_packet(client_halt, 0, 1234, ""));

    ui.write("\nExiting...");

    return 0;
}