#include "../include/ClientComm.hpp"
#include "../include/ClientUI.hpp"
#include "../include/Signal.hpp"

#include <atomic>
#include <iostream>
#include <regex>

#include <unistd.h>

bool should_quit = false;    // signal flag
bool server_exiting = false;

ClientUI ui;
ClientComm comm_manager;

void sig_int_handler(int signum)
{
    should_quit = true;
    ui.set_quit();
}

void assert_username_formatting(std::string username);
void* cmd_thread(void* args);
void* ntf_thread(void* args);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "usage: " << argv[0] << " @<username>" << std::endl;
        return EXIT_SUCCESS;
    }

    std::string username = std::string(argv[1]);
    assert_username_formatting(username);

    std::cout << "Initializing Artuiter..." << std::endl;

    // Establish connection with primary server
    comm_manager.init();

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

    std::cout << "Exiting..." << std::endl;;

    return 0;
}

// Assert username format (@<username>) and size (4–20) as per the specification
void assert_username_formatting(std::string username)
{
    if (!std::regex_match(username, std::regex("@[a-z0-9]{4,20}")))
    {
        if (username[0] != '@')
        {
            std::cout << "[ERROR] Must insert an at sign (@) before the username." << std::endl;
            exit(EXIT_FAILURE);
        }
        else if (username.length()-1 < 4 || username.length()-1 > 20)
        {
            std::cout << "[ERROR] Invalid username. Username must be between 4 and 20 characters long." << std::endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            std::cout << "[ERROR] Invalid username." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void* cmd_thread(void* args)
{
    int cmd_sockfd = comm_manager.get_cmd_sockfd();
    packet pkt;

    while(true)
    {
        // Read message (command) from user. If empty, ignore. If EOF, exit.
        std::string message = ui.read_command();

        // ctrl+C || exit command || ctrl+D
        if (should_quit || message == "EXIT" || message == "exit")
        {
            // Notify server that client is down
            comm_manager.write_pkt(cmd_sockfd, create_packet(client_halt, 0, 0, std::string()));
            break;
        }

        if (server_exiting)
        {
            // Notify server's cmd thread to exit
            comm_manager.write_pkt(cmd_sockfd, create_packet(server_halt, 0, 0, std::string()));
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

        // Server notified that it's exiting. Need to tell client cmd thread to notify server cmd thread
        else if (pkt.type == server_halt)
        {
            server_exiting = true;
            ui.set_quit();
        }
        
        // Add notification to client's UI feed
        else if (pkt.type == notification) ui.update_feed(pkt.payload);
    }

    return NULL;
}