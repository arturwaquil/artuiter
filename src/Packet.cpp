#include <cstring>
#include "Packet.hpp"

packet create_packet(EventType type, uint16_t seqn, uint16_t timestamp, std::string payload)
{
    packet pkt;
    pkt.type = type;
    pkt.seqn = seqn;
    pkt.timestamp = timestamp;
    strcpy(pkt.payload, payload.c_str());
    return pkt;
}
