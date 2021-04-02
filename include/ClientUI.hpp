#ifndef UI_HPP
#define UI_HPP

#include <list>
#include <string>

class ClientUI {
    public:
        ClientUI();
        std::string read();
        int write(std::string message);
    private:
        // ~ClientUI();
        std::list<std::string> printing_queue;
        pthread_mutex_t mutex;
};

#endif
