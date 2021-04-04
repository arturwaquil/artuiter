#ifndef NOTIFICATION_HPP
#define NOTIFICATION_HPP

#include <atomic>
#include <chrono>
#include <string>

#include <cstdint>

class Notification
{
    public:
        Notification(std::string _author, std::string _message, int _pending);

        std::string author;     // User who sent the message
        uint16_t id;            // Unique identifier
        int pending;            // Number of pending clients to receive
        std::string message;    // Content of the tweet
        
        std::chrono::_V2::system_clock::time_point timestamp;   // Time when received by server

};


#endif