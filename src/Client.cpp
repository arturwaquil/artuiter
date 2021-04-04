#include "../include/ClientComm.hpp"
#include "../include/ClientUI.hpp"
#include "../include/Signal.hpp"

#include <atomic>
#include <iostream>

std::atomic<bool> quit(false);    // signal flag
ClientUI ui;
ClientComm comm_manager;

void sigIntHandler(int signum)
{
    quit.store(true);
}

void* cmd_thread(void* args);
void* ntf_thread(void* args);

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

    // Initialize ncurses user interface
    ui.init();

    comm_manager.init(argv[2], argv[3], ui);

    // Set sigIntHandler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sigIntHandler);

    packet pkt;
    int cmd_sockfd = comm_manager.get_cmd_sockfd();

    // Send login message, wait for positive reply
    pkt = create_packet(login, 0, 0, username);
    comm_manager.write_pkt(cmd_sockfd, pkt);
    comm_manager.read_pkt(cmd_sockfd, &pkt);
    if (pkt.type == reply_login)
    {
        if (pkt.payload == std::string("OK"))
        {
            ui.update_feed("User " + username + " logged in successfully.");
        }
        else
        {
            ui.update_feed("[ERROR] Couldn't login, user " + username + " already has two connections to the server.");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        ui.update_feed("[ERROR] Couldn't login.");
        exit(EXIT_FAILURE);
    }

    ui.update_feed("Commands: FOLLOW @<username> | SEND <message> | EXIT");

    // Initialize separate command and notification threads
    pthread_t cmd_thd, ntf_thd;
    pthread_create(&cmd_thd, NULL, cmd_thread, NULL);
    pthread_create(&ntf_thd, NULL, ntf_thread, NULL);

    pthread_join(cmd_thd, NULL);
    pthread_join(ntf_thd, NULL);

    // Notify server that client is down
    comm_manager.write_pkt(cmd_sockfd, create_packet(client_halt, 0, 1234, ""));

    ui.update_feed("Exiting...");

    comm_manager.~ClientComm();
    ui.~ClientUI();
    
    return 0;
}

void* cmd_thread(void* args)
{
    int cmd_sockfd = comm_manager.get_cmd_sockfd();
    packet pkt;

    while(!quit.load())
    {
        // Read message (command) from user. If empty, ignore. If EOF, exit.
        // TODO: when SIGINT is received, getline() blocks the exit.
        std::string message = ui.read_command();

        if (quit.load()) break;

        if (std::cin.eof()) break;
        if (message.empty()) continue;

        // Handle exit command similarly to SIGINT
        if (message == "EXIT" || message == "exit") break;

        if (quit.load()) break;

        // Send command to server
        pkt = create_packet(command, 0, 1234, message);
        comm_manager.write_pkt(cmd_sockfd, pkt);

        if (quit.load()) break;

        // Receive server's reply to the command
        comm_manager.read_pkt(cmd_sockfd, &pkt);
        ui.update_feed(pkt.payload);
        if (pkt.payload == std::string("Unknown command.")) ui.update_feed("Commands: FOLLOW @<username> | SEND <message> | EXIT");
    }

    return NULL;
}

void* ntf_thread(void* args)
{
    int ntf_sockfd = comm_manager.get_ntf_sockfd();
    packet pkt;

    while(!quit.load())
    {
        comm_manager.read_pkt(ntf_sockfd, &pkt);
        if (pkt.type == notification) ui.update_feed(pkt.payload);
    }

    return NULL;
}