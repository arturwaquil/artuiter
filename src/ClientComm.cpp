#include "../include/ClientComm.hpp"

ClientComm::ClientComm(std::string _hostname, std::string _port, ClientUI _ui)
{
    hostname = _hostname;
    port = _port;
    ui = _ui;
    
    _create();
    _connect();
}

ClientComm::~ClientComm()
{
    close(socket_file_descriptor);
}

int ClientComm::get_sockfd()
{
    return socket_file_descriptor;
}

int ClientComm::read_pkt(packet* pkt)
{
    bzero(pkt, sizeof(*pkt));
    int n = read(socket_file_descriptor, pkt, sizeof(*pkt));
    if (n < 0) error("Couldn't read packet from socket.");
    return 0;
}

int ClientComm::write_pkt(packet pkt)
{
    int n = write(socket_file_descriptor, &pkt, sizeof(pkt));
    if (n < 0) error("Couldn't write packet to socket.");
    return 0;
}

int ClientComm::_create()
{
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) error("Couldn't create socket.");
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
    
    if (connect(socket_file_descriptor, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) error("Couldn't connect to server.");

    return 0;
}

// Print error message and exit with EXIT_FAILURE
void ClientComm::error(std::string error_message)
{
    ui.write("[ERROR] " + error_message + "\n\tErrno " + std::to_string(errno) + ": " + std::string(strerror(errno)));
    exit(errno);
}