#include "../include/GeneralComm.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

#include <unistd.h>

void GeneralComm::init()
{
    read_servers_info();

}

int GeneralComm::read_pkt(int socket, packet* pkt)
{
    bzero(pkt, sizeof(*pkt));
    int n = read(socket, pkt, sizeof(*pkt));
    if (n < 0) error("Couldn't read packet from socket.");
    return 0;
}

int GeneralComm::write_pkt(int socket, packet pkt)
{
    int n = write(socket, &pkt, sizeof(pkt));
    if (n < 0) error("Couldn't write packet to socket.");
    return 0;
}

// Retrieve servers' info (IP and port) from config file
void GeneralComm::read_servers_info()
{
    // Read JSON structure from file
    std::ifstream file("data/servers.json");
    nlohmann::json j;
    file >> j;
    file.close();

    for (auto item : j) servers_info[item["id"]] = std::make_pair(item["ip"], item["port"]);

    if (servers_info.empty()) error("Couldn't retrieve servers info.");
}

// Print error message and exit with errno
void GeneralComm::error(std::string error_message)
{
    std::cout << "[ERROR] " << error_message << "\n\tErrno " << std::to_string(errno) << ": " << std::string(strerror(errno)) << std::endl;
    exit(errno);
}