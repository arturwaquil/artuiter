#ifndef UI_HPP
#define UI_HPP

#include <list>
#include <mutex>
#include <string>

class UI {
    public:
        UI();
        std::string read();
        int write(std::string message);
    private:
        std::list<std::string> printing_queue;
        std::mutex mutex;
};

#endif
