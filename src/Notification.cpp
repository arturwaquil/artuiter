#include "../include/Notification.hpp"

std::atomic<uint16_t> NOTIF_ID(0);

Notification::Notification(std::string _author, std::string _message, int _pending)
{
    author = _author;
    message = _message;
    pending = _pending;
    id = NOTIF_ID.fetch_add(1);
    timestamp = std::chrono::system_clock::now();
}