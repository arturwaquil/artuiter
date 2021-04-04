#ifndef CLIENT_UI_HPP
#define CLIENT_UI_HPP

#include <list>
#include <string>

// This is, again, only the UI of the client. On server side, a simple std::cout does the job.
// Only works in a 24*80 screen...

typedef std::pair<int,int> position;

class ClientUI {
    public:
        ClientUI();
        ~ClientUI();
        void init();
        std::string read_command();
        int update_feed(std::string update);
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

        std::list<std::string> last_ten_updates;
};

#endif
