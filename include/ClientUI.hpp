#ifndef UI_HPP
#define UI_HPP

#include <iostream>
#include <string>
#include <list>
#include <mutex>

class ClientUI {
    public:
        ClientUI();
        std::string read();
        int write(std::string message);
    private:
        // ~ClientUI();
        std::list<std::string> printing_queue;
        pthread_mutex_t* mutex;
};

#endif
