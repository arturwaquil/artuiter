#include "../include/ServerComm.hpp"

packet create_packet(uint16_t type, uint16_t seqn, uint16_t timestamp, std::string payload)
{
    packet pkt;
    pkt.type = type;
    pkt.seqn = seqn;
    pkt.timestamp = timestamp;
    strcpy(pkt.payload, payload.c_str());
    return pkt;
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

int ServerComm::get_sockfd()
{
    return socket_file_descriptor;
}

int ServerComm::read_pkt(int socket, packet* pkt)
{
    bzero(pkt, sizeof(*pkt));
    int n = read(socket, pkt, sizeof(*pkt));
    if (n < 0)
    {
        std::cout << "[ERROR] Couldn't read packet from socket." << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

int ServerComm::write_pkt(int socket, packet pkt)
{
    int n = write(socket, &pkt, sizeof(pkt));
    if (n < 0)
    {
        std::cout << "[ERROR] Couldn't write packet to socket." << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

int ServerComm::_create()
{
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0)
    {
        std::cout << "[ERROR] Couldn't create socket." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Set socket option to allow address reuse
    int enable = 1;
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        std::cout << "[ERROR] Couldn't set socket option to allow address reuse." << std::endl;
        exit(EXIT_FAILURE);
    }

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
    if (binded < 0)
    {
        std::cout << "[ERROR] Couldn't bind server to port 4000." << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}

// Start listening for connections (initialize queue)
int ServerComm::_listen()
{
    if (listen(socket_file_descriptor, 3) < 0)
    {
        std::cout << "[ERROR] Couldn't put server to listen to connections." << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

// Accept the first in the queue of pending connections
int ServerComm::_accept()
{
    sockaddr_in client_address;
    socklen_t client_address_length;
    int new_sockfd = accept(socket_file_descriptor, (struct sockaddr *) &client_address, &client_address_length);
    if (new_sockfd < 0)
    {
        std::cout << "[ERROR] Couldn't accept first connection in the queue." << std::endl;
        exit(EXIT_FAILURE);
    }

    return new_sockfd;
}
