#include "../include/ClientUI.hpp"

#include "../include/Packet.hpp"

#include <iostream>
#include <list>
#include <string>

#include <ncurses.h>

// TODO: discover why after client execution the \n character seems to be only LF instead of CR/LF...

// Trick from here: https://stackoverflow.com/a/39960443
#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

bool ncurses_started = false;

ClientUI::ClientUI()
{
    _start_curses();
    quit = false;
}

ClientUI::~ClientUI()
{
    _end_curses();
}

void ClientUI::init()
{
    line_width = 70;
    command_pos = std::make_pair(19,5);
    feed_pos = std::make_pair(14,5);

    basic_screen();

}

std::string ClientUI::read_command()
{
    _move(command_pos);
    
    char ch;
    std::string input;

    timeout(100);   // Time (ms) that getch() waits before returning ERR
    
    // Read input from user until break condition (newline or ctrl+D)
    while (!quit)
    {
        ch = getch();

        // Because timeout is positive, getch() is non-blocking, and returns
        // ERR if no input is read. We want to continue waiting for input.
        if (ch == ERR) continue;

        // If ctrl+D (EOF) is received, break from loop and set quit flag to exit 
        // client. An "exit" command is returned so that the ctrl+D combination 
        // is handled the same way in the client
        else if (ch == CTRL('d'))
        {
            quit = true;
            return std::string("exit");
        }

        // On newline, ignore if no input was given. Break from loop otherwise
        else if (ch == '\n')
        {
            if (input.empty()) continue;
            else break;
        }

        // Handle backspace
        else if (ch == 127)
        {
            if (!input.empty())
            {
                input.pop_back();
                addch('\b');
                addch(' ');
                int y, x;
                getyx(stdscr, y, x);

                // If reached start of second line, move to end of first
                if (y == command_pos.first+1 && x == command_pos.second) 
                {
                    move(command_pos.first, command_pos.second+line_width);
                }
                else
                {
                    move(y, x-1);
                }
            }
        }

        // Ignore chars that surpass the maximum message size
        else if (input.size() == MAX_MESSAGE_SIZE) continue;

        // Add char to input string
        else
        {
            input.push_back(ch);

            // If reached end of first line, move to start of second
            if ((int) input.size() == line_width+1) move(command_pos.first+1, command_pos.second);

            addch(ch);
        }
    }

    // Clear command-input part of the screen
    print(command_pos, "");
    print(command_pos.first+1, command_pos.second, "");

    return input;
}

int ClientUI::update_feed(std::string update)
{
    feed_lock.lock();
    
    if ((int) update.size() <= line_width)
    {
        // If update fits the width, add it to the list
        last_ten_updates.push_front(update);
    }
    else
    {
        // If update is too big, add it in pieces of line_width chars
        for (int i = 0; i < (int) update.size(); i += line_width)
        {
            if ((int) update.size() > i+line_width)
            {
                last_ten_updates.push_front(update.substr(i, line_width));
            }
            else
            {
                last_ten_updates.push_front(update.substr(i));
            }
        }
    }

    // Keep only the 10 most recent updates
    while ((int) last_ten_updates.size() > 10) last_ten_updates.pop_back();

    // Print last ten updates bottom-up in the feed
    int row = feed_pos.first;
    for (std::string u : last_ten_updates) 
    {
        print(row, feed_pos.second, u);
        row--;
    }

    feed_lock.unlock();
    
    return 0;
}

bool ClientUI::quit_flag()
{
    return quit;
}

void ClientUI::set_quit()
{
    quit = true;
    _end_curses();
}

void ClientUI::_start_curses()
{
    if (ncurses_started)
    {
        refresh();
    }
    else
    {
        initscr();
        noecho();   // Do not echo pressed chars on screen (this is done manually)
        ncurses_started = true;
    }

}

void ClientUI::_end_curses()
{
    if (ncurses_started && !isendwin())
    {
        ncurses_started = false;
        endwin();
    }
}

void ClientUI::basic_screen()
{
    // Save previous position
    int old_y, old_x;
    getyx(stdscr, old_y, old_x);

    clear();

    move( 0,0); addstr("                                                                                ");
    move( 1,0); addstr("                              Welcome to Artuiter!                              ");
    move( 2,0); addstr("                                                                                ");
    move( 3,0); addstr("     YOUR FEED                                                                  ");
    move( 4,0); addstr("   **************************************************************************   ");
    move( 5,0); addstr("   *                                                                        *   ");
    move( 6,0); addstr("   *                                                                        *   ");
    move( 7,0); addstr("   *                                                                        *   ");
    move( 8,0); addstr("   *                                                                        *   ");
    move( 9,0); addstr("   *                                                                        *   ");
    move(10,0); addstr("   *                                                                        *   ");
    move(11,0); addstr("   *                                                                        *   ");
    move(12,0); addstr("   *                                                                        *   ");
    move(13,0); addstr("   *                                                                        *   ");
    move(14,0); addstr("   *                                                                        *   ");
    move(15,0); addstr("   **************************************************************************   ");
    move(16,0); addstr("                                                                                ");
    move(17,0); addstr("     SEND COMMAND ( FOLLOW @<username> | SEND <message> | EXIT )                ");
    move(18,0); addstr("   **************************************************************************   ");
    move(19,0); addstr("   *                                                                        *   ");
    move(20,0); addstr("   *                                                                        *   ");
    move(21,0); addstr("   **************************************************************************   ");
    move(22,0); addstr("                                                                                ");
    move(23,0); addstr("                                                                                ");

    // Move back to previous position
    move(old_y, old_x);
}

void ClientUI::print(int row, int col, std::string s)
{
    // Save previous position
    int old_y, old_x;
    getyx(stdscr, old_y, old_x);

    // Fill the end of the line based on the basic screen
    std::string rest = std::string(" *   ");
    rest.insert(0, line_width-s.size(), ' ');

    // Print string in desired position
    move(row, col);
    addstr((s + rest).c_str());

    // Move back to previous position
    move(old_y, old_x);
}

void ClientUI::print(position pos, std::string s)
{
    print(pos.first, pos.second, s);
}

void ClientUI::_move(position pos)
{
    move(pos.first, pos.second);
}