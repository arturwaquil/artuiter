#include "../include/Packet.hpp"

#include <string>

#include <cstring>

packet create_packet(EventType type, uint16_t seqn, uint16_t timestamp, std::string payload)
{
    packet pkt;
    pkt.type = type;
    pkt.seqn = seqn;
    pkt.timestamp = timestamp;
    memset(pkt.payload, 0, MAX_MESSAGE_SIZE);
    strcpy(pkt.payload, payload.c_str());
    return pkt;
}
