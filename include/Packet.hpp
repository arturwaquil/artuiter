#ifndef PACKET_HPP
#define PACKET_HPP

#include <string>

#define MAX_MESSAGE_SIZE 128

typedef enum
{
    // client to server
    login,
    command,
    client_halt,
    reply_notification,
    ask_primary,

    // server to client
    notification,
    server_halt,
    reply_login,
    reply_command,
    reply_primary,

    // server to server (election-related)
    election_begin,
    election_answer,
    election_coordinator

} EventType;

typedef struct _packet
{
    EventType type;
    uint16_t seqn;
    uint16_t timestamp;
    char payload[MAX_MESSAGE_SIZE];

} packet;

packet create_packet(EventType type, uint16_t seqn, uint16_t timestamp, std::string payload);


#endif