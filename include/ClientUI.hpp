#ifndef CLIENT_UI_HPP
#define CLIENT_UI_HPP

#include "Typedefs.hpp"

#include <list>
#include <mutex>
#include <string>

class ClientUI {
    public:
        ClientUI();
        ~ClientUI();
        void init();
        std::string read_command();
        int update_feed(std::string update);

        bool quit_flag();
        void set_quit();

    private:
        void _start_curses();
        void _end_curses();

        void basic_screen();

        void print(int row, int col, std::string s);
        void print(position pos, std::string s);

        void _move(position pos);

        position command_pos;
        position feed_pos;

        int line_width;

        str_list last_ten_updates;
        std::mutex feed_lock;

        bool quit;
};

#endif
