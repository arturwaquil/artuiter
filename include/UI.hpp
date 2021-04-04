#ifndef UI_HPP
#define UI_HPP

#include <list>
#include <string>

// TODO: use a better interface that allows to read and write simultaneously...

class UI {
    public:
        UI();
        ~UI();
        std::string read();
        int write(std::string message);
    private:
        std::list<std::string> printing_queue;
        pthread_mutex_t mutex;
};

#endif
