#ifndef PACKET_HPP
#define PACKET_HPP

#include <string>

typedef enum
{
    // client to server
    login,
    command,
    client_halt,
    reply_notification,

    // server to client
    notification,
    server_halt,
    reply_login,
    reply_command

} EventType;

typedef struct _packet
{
    EventType type;
    uint16_t seqn;
    uint16_t timestamp;
    char payload[256];

} packet;

packet create_packet(EventType type, uint16_t seqn, uint16_t timestamp, std::string payload);


#endif