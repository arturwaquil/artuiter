#include "../include/ClientUI.hpp"

#include <iostream>
#include <list>
#include <string>

#include <ncurses.h>

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
    command_pos = std::make_pair(20,5);
    feed_pos = std::make_pair(15,5);

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

        // If ctrl+D (EOF) is received, break from loop and set quit flag to exit client
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
                move(y, x-1);
            }
        }

        // Add char to input string
        else
        {
            input.push_back(ch);
            addch(ch);
        }
    }

    // Clear command-input part of the screen
    print(command_pos, "");

    return input;
}

int ClientUI::update_feed(std::string update)
{
    feed_lock.lock();
    
    // Insert update in list, ensure max size of 10
    last_ten_updates.push_front(update);
    if (last_ten_updates.size() > 10) last_ten_updates.pop_back();

    // Print last ten updates
    // TODO: this if-else part will depend on the max length of the tweet
    // TODO: ensure no overflow at the top of the feed box
    int row = 15;
    for (std::string u : last_ten_updates)
    {
        int len = u.size();

        if (len <= line_width) 
        {
            print(row, 5, u);
            row--;
        }
        else if (len <= 2*line_width)
        {
            print(row-1, 5, u.substr(0,line_width));
            print(row, 5, u.substr(line_width));
            row -= 2;
        }
        else //if (len <= 3*line_width)
        {
            print(row-2, 5, u.substr(0,line_width));
            print(row-1, 5, u.substr(line_width,line_width));
            print(row, 5, u.substr(2*line_width));
            row -= 3;
        }

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
    move( 3,0); addstr("                                                                                ");
    move( 4,0); addstr("     YOUR FEED                                                                  ");
    move( 5,0); addstr("   **************************************************************************   ");
    move( 6,0); addstr("   *                                                                        *   ");
    move( 7,0); addstr("   *                                                                        *   ");
    move( 8,0); addstr("   *                                                                        *   ");
    move( 9,0); addstr("   *                                                                        *   ");
    move(10,0); addstr("   *                                                                        *   ");
    move(11,0); addstr("   *                                                                        *   ");
    move(12,0); addstr("   *                                                                        *   ");
    move(13,0); addstr("   *                                                                        *   ");
    move(14,0); addstr("   *                                                                        *   ");
    move(15,0); addstr("   *                                                                        *   ");
    move(16,0); addstr("   **************************************************************************   ");
    move(17,0); addstr("                                                                                ");
    move(18,0); addstr("     SEND COMMAND ( FOLLOW @<username> | SEND <message> | EXIT )                ");
    move(19,0); addstr("   **************************************************************************   ");
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