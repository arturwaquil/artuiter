#include "../include/ClientUI.hpp"

ClientUI::ClientUI() {
    printing_queue = std::list<std::string>();
    pthread_mutex_init(mutex, NULL);
}

std::string ClientUI::read()
{
    pthread_mutex_lock(mutex);
    std::cout << "> ";
    std::string message;
    std::cin >> message;
    pthread_mutex_unlock(mutex);

    return message;
}

int ClientUI::write(std::string message)
{
    pthread_mutex_lock(mutex);
    std::cout << message << std::endl;
    pthread_mutex_unlock(mutex);

    return 0;
}