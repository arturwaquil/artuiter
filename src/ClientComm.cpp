#include "../include/ClientComm.hpp"

#include "../include/Packet.hpp"

#include <iostream>
#include <map>
#include <string>

#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void ClientComm::init()
{
    GeneralComm::init();

    _create();
    _connect_to_primary();
}

ClientComm::~ClientComm()
{
    _disconnect();
}

int ClientComm::get_cmd_sockfd()
{
    return cmd_sockfd;
}

int ClientComm::get_ntf_sockfd()
{
    return ntf_sockfd;
}

int ClientComm::_create()
{
    cmd_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ntf_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (cmd_sockfd < 0 || ntf_sockfd < 0) error("Couldn't create socket.");
    return 0;
}

int ClientComm::_connect_to_primary()
{
    int connected_id;
    std::string ip, port;
    bool cant_connect;
    
    // Try connection to servers, following config file's priority order
    for (auto server : servers_info)
    {
        int id = server.first;
        ip = server.second.first;
        port = server.second.second;

        if (_connect(ip, port))
        {
            cant_connect = false;
            connected_id = id;
            break;
        }
    }
    if (cant_connect) error("All servers seem to be down.");

    // Ask connected server what is the primary server, wait for response with its id 
    packet pkt = create_packet(ask_primary, 0, 0, std::string());
    write_pkt(cmd_sockfd, pkt);
    read_pkt(cmd_sockfd, &pkt);

    if (pkt.type != reply_primary) error("Failed to retrieve primary server info.");

    // If connected server is not the primary, disconnect from it and connect to the right one
    int primary_id = atoi(pkt.payload);
    if (connected_id != primary_id)
    {
        _disconnect();
        auto server = servers_info[primary_id];
        ip = server.first;
        port = server.second;
        _connect(ip, port);
    }
}

int ClientComm::_connect(std::string hostname, std::string port)
{
    // Get server representation from hostname
    hostent *server;
    server = gethostbyname(hostname.c_str());
    if (server == NULL) error("Couldn't get server by hostname.");

    // Set server address info
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(stoi(port));
    server_address.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(server_address.sin_zero), 8);     
    
    if (connect(cmd_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) error("Couldn't connect to server.");
    if (connect(ntf_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) error("Couldn't connect to server.");

    return 0;
}

void ClientComm::_disconnect()
{
    close(cmd_sockfd);
    close(ntf_sockfd);
}