#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct _packet
{
    uint16_t type;  // DATA or CMD. TODO: is it necessary?
    uint16_t seqn;
    uint16_t timestamp;
    char payload[256];

} packet;

int main()
{
    int PORT = 4000;
    
    // Create general socket
    int general_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (general_sockfd < 0) exit(1);
    int enable = 1;
    if (setsockopt(general_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) exit(6);
    
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_address.sin_zero), 8);     
    
    // Bind general socket to the desired PORT
    int binded = bind(general_sockfd, (struct sockaddr *) &server_address, sizeof(server_address));
    if (binded < 0) exit(2);

    // Start listening for connections
    listen(general_sockfd, 3);

    // Accept the first connection in line
    sockaddr_in client_address;
    socklen_t client_address_length;
    int new_sockfd = accept(general_sockfd, (struct sockaddr *) &client_address, &client_address_length);
    if (new_sockfd < 0) exit(3);

    int n;
    packet pkt;
    while(strcmp(pkt.payload, "exit") != 0)
    {
        memset(&pkt, 0, sizeof(pkt));

        // Read the message from the client
        n = read(new_sockfd, &pkt, sizeof(pkt));
        if (n < 0) exit(4);

        std::cout << "type: " << pkt.type << std::endl;
        std::cout << "seqn: " << pkt.seqn << std::endl;
        std::cout << "timestamp: " << pkt.timestamp << std::endl;
        std::cout << "payload: " << pkt.payload << std::endl;

        // Send reply back to the client
        std::string message = "Message received!";
        n = write(new_sockfd, message.c_str(), message.length());
        if (n < 0) exit(5);
    }

    // Close sockets
    close(new_sockfd);
    close(general_sockfd);

    return 0;
}