#include "../include/UI.hpp"

#include <iostream>
#include <list>
#include <mutex>
#include <string>

UI::UI() {
    printing_queue = std::list<std::string>();
}

std::string UI::read()
{
    mutex.lock();
    std::cout << "> ";
    std::string message;
    std::getline(std::cin, message);
    mutex.unlock();

    return message;
}

int UI::write(std::string message)
{
    mutex.lock();
    std::cout << message << std::endl;
    mutex.unlock();

    return 0;
}