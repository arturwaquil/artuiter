#include "../include/UI.hpp"

#include <iostream>
#include <list>
#include <string>

UI::UI() {
    printing_queue = std::list<std::string>();
    pthread_mutex_init(&mutex, NULL);
}

UI::~UI() {
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
}

std::string UI::read()
{
    pthread_mutex_lock(&mutex);
    std::cout << "> ";
    std::string message;
    std::getline(std::cin, message);
    pthread_mutex_unlock(&mutex);

    return message;
}

int UI::write(std::string message)
{
    pthread_mutex_lock(&mutex);
    std::cout << message << std::endl;
    pthread_mutex_unlock(&mutex);

    return 0;
}