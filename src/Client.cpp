#include "../include/ClientComm.hpp"
#include "../include/ClientUI.hpp"
#include "../include/Signal.hpp"

#include <atomic>
#include <iostream>
#include <regex>

#include <unistd.h>

bool should_quit = false;    // signal flag
ClientUI ui;
ClientComm comm_manager;

void sig_int_handler(int signum)
{
    should_quit = true;
    ui.set_quit();
}

void* cmd_thread(void* args);
void* ntf_thread(void* args);

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        std::cout << "usage: " << argv[0] << " @<username> <hostname> <port>" << std::endl;
        return EXIT_SUCCESS;
    }

    std::string username = std::string(argv[1]);

    // Assert username format (@<username>) and size (4–20) as per the specification
    if (!std::regex_match(username, std::regex("@[a-z]{4,20}")))
    {
        if (username[0] != '@')
        {
            std::cout << "[ERROR] Must insert an at sign (@) before the username." << std::endl;
            return EXIT_FAILURE;
        }
        else if (username.length()-1 < 4 || username.length()-1 > 20)
        {
            std::cout << "[ERROR] Invalid username. Username must be between 4 and 20 characters long." << std::endl;
            return EXIT_FAILURE;
        }
        else
        {
            std::cout << "[ERROR] Invalid username." << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Initializing client..." << std::endl;

    // Connect to the server
    comm_manager.init(argv[2], argv[3]);

    // Set sig_int_handler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sig_int_handler);

    packet pkt;
    int cmd_sockfd = comm_manager.get_cmd_sockfd();

    // Send login message to server, assert that reply is positive
    pkt = create_packet(login, 0, 0, username);
    comm_manager.write_pkt(cmd_sockfd, pkt);
    comm_manager.read_pkt(cmd_sockfd, &pkt);
    if (pkt.type != reply_login)
    {
        std::cout << "[ERROR] Couldn't login." << std::endl;
        return EXIT_FAILURE;
    }
    else if (pkt.payload != std::string("OK"))
    {
        std::cout << "[ERROR] Couldn't login, user " << username << " already has two connections to the server." << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize ncurses user interface
    ui.init();
    ui.update_feed("User " + username + " logged in successfully.");

    // Initialize separate command and notification threads
    pthread_t cmd_thd, ntf_thd;
    pthread_create(&cmd_thd, NULL, cmd_thread, NULL);
    pthread_create(&ntf_thd, NULL, ntf_thread, NULL);

    // Wait for both threads to finish
    pthread_join(cmd_thd, NULL);
    pthread_join(ntf_thd, NULL);

    ui.update_feed("Exiting...");

    return 0;
}

void* cmd_thread(void* args)
{
    int cmd_sockfd = comm_manager.get_cmd_sockfd();
    packet pkt;

    while(!should_quit)
    {
        // Read message (command) from user. If empty, ignore. If EOF, exit.
        std::string message = ui.read_command();

        // ctrl+C || exit command
        if (should_quit || message == "EXIT" || message == "exit")
        {
            // Notify server that client is down
            comm_manager.write_pkt(cmd_sockfd, create_packet(client_halt, 0, 0, std::string()));
            break;
        }

        if (message.empty()) continue;

        // Send command to server
        pkt = create_packet(command, 0, 1234, message);
        comm_manager.write_pkt(cmd_sockfd, pkt);

        if (should_quit)
        {
            // Notify server that client is down
            comm_manager.write_pkt(cmd_sockfd, create_packet(client_halt, 0, 0, std::string()));
            break;
        }

        // Receive server's reply to the command
        comm_manager.read_pkt(cmd_sockfd, &pkt);
        ui.update_feed(pkt.payload);
    }

    return NULL;
}

void* ntf_thread(void* args)
{
    int ntf_sockfd = comm_manager.get_ntf_sockfd();
    packet pkt;

    while(!ui.quit_flag())
    {
        comm_manager.read_pkt(ntf_sockfd, &pkt);

        // If cmd thread sent halt signal to the server and the server notified the ntf thread
        if (pkt.type == client_halt) break;
        
        // Add notification to client's UI feed
        else if (pkt.type == notification) ui.update_feed(pkt.payload);
    }

    return NULL;
}