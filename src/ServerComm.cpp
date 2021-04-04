#include "../include/ServerComm.hpp"

#include "../include/UI.hpp"
#include "../include/Packet.hpp"

#include <iostream>
#include <string>

#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

client_thread_params create_client_thread_params(std::string username, std::pair<int,int> sockets)
{
    client_thread_params ctp;
    ctp.username = username;
    ctp.sockets = sockets;
    return ctp;
}

ServerComm::ServerComm()
{
    port = 4000;
    
    _create();
    _bind();
    _listen();
}

ServerComm::~ServerComm()
{
    close(socket_file_descriptor);
}

void ServerComm::set_ui(UI _ui)
{
    ui = _ui;
}

int ServerComm::get_sockfd()
{
    return socket_file_descriptor;
}

int ServerComm::read_pkt(int socket, packet* pkt)
{
    bzero(pkt, sizeof(*pkt));
    int n = read(socket, pkt, sizeof(*pkt));
    if (n < 0) error("Couldn't read packet from socket.");
    return 0;
}

int ServerComm::write_pkt(int socket, packet pkt)
{
    int n = write(socket, &pkt, sizeof(pkt));
    if (n < 0) error("Couldn't write packet to socket.");
    return 0;
}

int ServerComm::_create()
{
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) error("Couldn't create socket.");

    // Set socket option to allow address reuse
    int enable = 1;
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        error("Couldn't set socket option to allow address reuse.");

    return 0;
}

// Bind server to the desired port (hardcoded port 4000)
int ServerComm::_bind()
{
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_address.sin_zero), 8);     
    
    int binded = bind(socket_file_descriptor, (struct sockaddr *) &server_address, sizeof(server_address));
    if (binded < 0) error("Couldn't bind server to port 4000.");

    return 0;
}

// Start listening for connections (initialize queue)
int ServerComm::_listen()
{
    if (listen(socket_file_descriptor, 3) < 0) error("Couldn't put server to listen to connections.");
    return 0;
}

// Accept the first in the queue of pending connections
std::pair<int,int> ServerComm::_accept()
{
    sockaddr_in client_address;
    socklen_t client_address_length = sizeof(struct sockaddr_in);

    // The tests for quit flag is done to make sure the server exits with no error when SIGINT is received.

    // Accept first connection request, get client address
    int cmd_sockfd = accept(socket_file_descriptor, (struct sockaddr *) &client_address, &client_address_length);
    if (quit) return std::make_pair(0,0);

    // Accept next connection request from the same client address    
    int ntf_sockfd = accept(socket_file_descriptor, (struct sockaddr *) &client_address, &client_address_length);
    if (quit) return std::make_pair(0,0);

    if (cmd_sockfd < 0 || ntf_sockfd < 0) error("Couldn't accept first connection in the queue.");

    return std::make_pair(cmd_sockfd, ntf_sockfd);
}

// Simply set the quit flag to allow the correct return in _accept()
void ServerComm::set_quit()
{
    quit = true;
}

// Print error message and exit with EXIT_FAILURE
void ServerComm::error(std::string error_message)
{
    ui.write("[ERROR] " + error_message + "\n\tErrno " + std::to_string(errno) + ": " + std::string(strerror(errno)));
    exit(errno);
}