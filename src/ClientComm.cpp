#include "../include/ClientComm.hpp"

#include "../include/UI.hpp"
#include "../include/Packet.hpp"

#include <string>

#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

ClientComm::ClientComm()
{

}

void ClientComm::init(std::string _hostname, std::string _port, UI _ui)
{
    hostname = _hostname;
    port = _port;
    ui = _ui;
    
    _create();
    _connect();
}

ClientComm::~ClientComm()
{
    close(cmd_sockfd);
    close(ntf_sockfd);
}

int ClientComm::get_cmd_sockfd()
{
    return cmd_sockfd;
}

int ClientComm::get_ntf_sockfd()
{
    return ntf_sockfd;
}

int ClientComm::read_pkt(int socket, packet* pkt)
{
    bzero(pkt, sizeof(*pkt));
    int n = read(socket, pkt, sizeof(*pkt));
    if (n < 0) error("Couldn't read packet from socket.");
    return 0;
}

int ClientComm::write_pkt(int socket, packet pkt)
{
    int n = write(socket, &pkt, sizeof(pkt));
    if (n < 0) error("Couldn't write packet to socket.");
    return 0;
}

int ClientComm::_create()
{
    cmd_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ntf_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (cmd_sockfd < 0 || ntf_sockfd < 0) error("Couldn't create socket.");
    return 0;
}

int ClientComm::_connect()
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

// Print error message and exit with EXIT_FAILURE
void ClientComm::error(std::string error_message)
{
    ui.write("[ERROR] " + error_message + "\n\tErrno " + std::to_string(errno) + ": " + std::string(strerror(errno)));
    exit(errno);
}